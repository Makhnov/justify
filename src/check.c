/**
 * @file check.c
 * @brief Implémentations du module de vérification d'entrée.
 * @see check.h pour des informations plus détaillées sur les structures et fonctions publiques.
 */

#include "check.h"

/** @brief Voir @ref VALID_CHARSETS : liste blanche des encodages acceptés (terminée par NULL). */
char *VALID_CHARSETS[] = {
    "iso-8859-1",
    "us-ascii", // cf. Lisez-Moi.txt
    NULL,
};

/** @brief Implémentation de @ref fileBI : lance `file -bi`, redirige STDOUT/STDERR via pipes, et décide. */
int fileBI(char *path, char *buffer, size_t s) {
    // On prépare une pipe pour capturer la sortie de la commande (STDOUT du fils)
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        snprintf(buffer, s, "Échec de pipe: %s (errno %d)\n", strerror(errno), errno);
        return 0;
    }

    // On prépare une seconde pipe pour capturer proprement les erreurs du fils (STDERR)
    int pipe_err[2];
    if (pipe(pipe_err) == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        snprintf(buffer, s, "Échec de pipe_err: %s (errno %d)\n", strerror(errno), errno);
        return 0;
    }

    // On créé un processus fils
    int pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        close(pipe_err[0]);
        close(pipe_err[1]);
        snprintf(buffer, s, "Échec de fork: %s (errno %d)\n", strerror(errno), errno);
        return 0;
    }

    if (!pid) {
        /* PROCESSUS FILS */

        // Fermer les extrémités lecture
        close(pipefd[0]);
        close(pipe_err[0]);

        // Rediriger STDOUT -> pipefd[1]
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            dprintf(pipe_err[1], "Échec de dup2 pour STDOUT: %s (errno %d)\n", strerror(errno), errno);
            close(pipefd[1]);
            close(pipe_err[1]);
            _exit(1);
        }

        // Rediriger STDERR -> pipe_err[1]
        if (dup2(pipe_err[1], STDERR_FILENO) == -1) {
            dprintf(pipe_err[1], "Échec de dup2 pour STDERR: %s (errno %d)\n", strerror(errno), errno);
            close(pipefd[1]);
            close(pipe_err[1]);
            _exit(1);
        }

        // Fermer les anciennes extrémités écriture
        close(pipefd[1]);
        close(pipe_err[1]);

        // Exécuter `file -bi -- <path>`
        execlp("file", "file", "-bi", "--", path, NULL);

        // Si on arrive ici, execlp a échoué : écrire sur STDERR (redirigée) puis sortir
        dprintf(STDERR_FILENO, "Échec de execlp: %s (errno %d)\n", strerror(errno), errno);
        _exit(1);
    }

    /* PROCESSUS PERE */

    int status;

    // Fermer les extrémités écriture
    close(pipefd[1]);
    close(pipe_err[1]);

    // Lire STDOUT dans buffer et NUL-terminer
    ssize_t n = read(pipefd[0], buffer, s - 1);
    if (n == -1) {
        int read_errno = errno;
        close(pipefd[0]);
        close(pipe_err[0]);

        if (waitpid(pid, &status, 0) == -1) {
            snprintf(buffer, s, "Échec de read(): %s (errno %d)\nÉchec de waitpid(): %s (errno %d)\n",
                     strerror(read_errno), read_errno, strerror(errno), errno);
            return 0;
        }
        snprintf(buffer, s, "Échec de read(): %s (errno %d)\n", strerror(read_errno), read_errno);
        return 0;
    }
    buffer[n] = '\0';
    close(pipefd[0]);

    // Attendre le fils
    status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        snprintf(buffer, s, "Échec de waitpid(): %s (errno %d)\n", strerror(errno), errno);
        close(pipe_err[0]);
        return 0;
    }

    // Terminaison anormale (signal)
    if (!WIFEXITED(status)) {
        int signal = WTERMSIG(status);
        snprintf(buffer, s, "Le processus fils a été tué par un signal %d\n", signal);
        close(pipe_err[0]);
        return 0;
    }

    // Lire STDERR du fils : si présent => message d'erreur (remplace buffer) et 0
    ssize_t m = read(pipe_err[0], buffer, s - 1);
    close(pipe_err[0]);
    if (m == -1) {
        snprintf(buffer, s, "Échec de read() sur la pipe d'erreur: %s (errno %d)\n", strerror(errno), errno);
        return 0;
    }
    if (m > 0) {
        buffer[m] = '\0';
        return 0;
    }

    // Code de retour non nul sans message exploitable
    if (WEXITSTATUS(status)) {
        snprintf(buffer, s, "La commande 'file -bi %s' a échoué (code %d)\n", path, WEXITSTATUS(status));
        return 0;
    }

    // Validation charset via VALID_CHARSETS (strstr)
    int valide = 0;
    for (int i = 0; VALID_CHARSETS[i]; ++i) {
        if (strstr(buffer, VALID_CHARSETS[i]) != NULL) {
            valide = 1;
            break;
        }
    }

    if (!valide) {
        if (strstr(buffer, "inode") != NULL) {
            snprintf(buffer, s, "Le fichier n'est pas un fichier régulier\n");
            return 0;
        }
        if (strstr(buffer, "text") == NULL) {
            snprintf(buffer, s, "Le fichier n'est pas au format texte\n");
            return 0;
        }
        snprintf(buffer, s, "Le fichier n'est pas au format ISO-8859-1\n");
        return 0;
    }

    // Succès (buffer contient la ligne de `file -bi`, NUL-terminée)
    return 1;
}

/** @brief Implémentation de @ref getSize : interroge `fstat()`, ferme @p fd en cas d'échec/taille nulle. */
size_t getSize(int fd, char *filename) {
    struct stat st;
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "AODjustify ERROR> Impossible d'obtenir la taille de '%s' (%s)\n",
                filename, strerror(errno));
        close(fd);
        return 0;
    }
    if (!st.st_size) {
        fprintf(stderr, "AODjustify ERROR> Le fichier est vide\n");
        close(fd);
        return 0;
    }
    return (size_t)st.st_size;
}
