/**
 * @file main.c
 * @brief Point d'entrée d’AODjustify : vérifications E/S, orchestration du parsing et de la justification.
 *
 * @details
 * **Chaîne de traitement (correspond aux sections du code) :**
 * 1. **Arguments** : attend exactement 2 arguments utilisateur — `M` (Mu, entier > 0) et un *préfixe de fichier*.
 * 2. **Fichier d’entrée** : construit `<prefix>.in`, ouvre en lecture (`O_RDONLY | O_NONBLOCK`), récupère la taille
 *    via `getSize()`, et valide le type/encodage (texte ISO-8859-1) via `fileBI()` (message d’erreur mis dans `buffer`).
 * 3. **Lecture/Parsing** :
 *    - ouvre `<prefix>.out` en écriture binaire ;
 *    - crée le contexte mmap via `psopen(fd, size, Mu)` ;
 *    - pré-calcule le tableau `cubes[0..Mu]` avec `i^3` en 64 bits puis **saturation** sur `LLONG_MAX`.
 *    - boucle *par paragraphe* : `nextPG()` → `getPG()` ; entre paragraphes, écrit une **ligne vide** (`'\n'`).
 * 4. **Justification** : pour chaque paragraphe, appelle `setPG(&pg, Mu, out, cubes)` (DP + écriture des lignes).
 *    En cas d’échec du programme, le fichier de sortie en cours est **supprimé** via @ref `freeALL`.
 * 5. **Nettoyage & résultat** : `freeALL(ps, out, cubes)` ; affiche sur `stderr` la norme totale sous la forme
 *    `AODjustify CORRECT> <valeur>` puis retourne 0 en cas de succès (1 sinon).
 *
 * @note Noms de fichiers :
 * - entrée : `<prefix>.in`
 * - sortie : `<prefix>.out`
 *
 * @see check.h  (getSize, fileBI)
 * @see parse.h  (PS, PG, psopen, nextPG, getPG, freePG)
 * @see justify.h (setPG)
 * @see out.h    (writeLN, debuff)
 */

#include "check.h"
#include "justify.h"

#include <fcntl.h>
#include <limits.h>

/** @brief Fonction principale.
 *
 * @param argc Nombre d'arguments. Usage attendu : **3** (`argv[0]`, `M`, `prefix`).
 * @param argv `argv[1] = M` (Mu strictement positif), `argv[2] = prefix` (sans extension).
 * @return Code de retour : **0** si succès ; **1** en cas d'erreur (messages sur `stderr`).
 *
 * @details
 * - Construit les chemins `<prefix>.in` et `<prefix>.out`.
 * - Valide l’entrée (existence, taille, type/encodage ISO-8859-1).
 * - Ouvre/mappe l’entrée, prépare la sortie.
 * - Pré-calcule `cubes[0..M]` avec saturation sur `LLONG_MAX`.
 * - Itère par paragraphe : `nextPG()` → `getPG()` → `setPG()` ; insère une **ligne vide** entre paragraphes.
 * - En cas d’erreur `getPG()`, supprime le fichier de sortie en cours (`remove(file_out)`).
 * - Agrège la norme totale (`badness`) et l’affiche sur `stderr`.
 */
int main(int argc, char **argv) {
    int fd, fb;
    size_t size;
    char buffer[BUFFER_SIZE];
    char file_in[BUFFER_SIZE];
    char file_out[BUFFER_SIZE];

    /***********************************************/
    /*    Étape 1. Vérification des arguments      */
    /***********************************************/

    // 1.a. Vérification du bon respect de l'usage
    if (argc != 3) {
        fprintf(stderr, "AODjustify ERROR> Usage: ./AODjustify <M> <chemin-fichier>\n");
        return 1;
    }

    char *file_prefix = argv[2];
    char *M_string = argv[1];

    // 1.b. Vérification que M est strictement positif
    const int Mu = atoi(M_string);
    if (Mu <= 0) {
        fprintf(stderr, "AODjustify ERROR> M doit être un entier strictement positif\n");
        return 1;
    }

    /***********************************************/
    /*      Étape 2. Vérification du fichier       */
    /***********************************************/

    // 2.a. On vérifie que le fichier existe et que le programme peut le lire, on récupère sa taille au passage
    snprintf(file_in, BUFFER_SIZE, "%s.in", file_prefix);
    fd = open(file_in, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "AODjustify ERROR> Ouverture du fichier '%s' impossible (%s)\n", file_in, strerror(errno));
        return 1;
    }
    size = getSize(fd, file_in);
    if (!size) {
        // Les messages d'erreurs sont gérés dans getSize
        return 1;
    }

    // 2.b. Une fois ouvert, on vérifie que le fichier est bien un fichier texte, encodé au format ISO-8859-1 en utilisant la commande 'file -bi'
    fb = fileBI(file_in, buffer, sizeof(buffer));
    if (!fb) {
        fprintf(stderr, "AODjustify ERROR> %s", buffer);
        close(fd);
        return 1;
    }

    /***********************************************/
    /*  Étape 3. Lecture (parsing par paragraphe)  */
    /***********************************************/

    // 3.a. On ouvre et mappe le fichier en mémoire
    snprintf(file_out, BUFFER_SIZE, "%s.out", file_prefix);
    FILE *out = fopen(file_out, "wb");
    if (!out) {
        fprintf(stderr, "AODjustify ERROR> Impossible d'ouvrir le fichier de sortie\n");
        return 1;
    }

    PS *ps = psopen(fd, size, Mu);
    if (!ps) {
        fprintf(stderr, "AODjustify ERROR> Impossible d'ouvrir/mmap le fichier d'entrée\n");
        remove(file_out);
        fclose(out);
        return 1;
    }

    // 3.b. Petit helper pour optimisation, on calcule au préalable tous les cubes possibles
    long long csize = (long long)(Mu + 1);
    long long *cubes = (long long *)malloc((size_t)csize * sizeof(long long));
    if (!cubes) {
        fprintf(stderr, "AODjustify ERROR> Mémoire insuffisante pour le calcul des cubes\n");
        remove(file_out);
        psclose(ps);
        fclose(out);
        return 1;
    }
    for (long long i = 0; i < csize; ++i) {
        long long t = (long long)i * i * i;
        cubes[i] = (t > LLONG_MAX) ? LLONG_MAX : (long long)t;
    }

    // 3.c. Boucle principale qui itère sur tous les paragraphes du texte

    /** @brief Un petit clin d'œil aux ancêtres : Badness représente la "mocheté" totale de notre justification. The lowest the badness is, the prettier the text will look! */
    long long badness = 0;

    int not_first = 0;
    while (1) {
        int has = nextPG(ps); // 0 = fin ; 1 = paragraphe prêt ; <0 = erreur
        if (has < 0) {
            fprintf(stderr, "AODjustify ERROR> Erreur lors du parsing\n");
            freeALL(ps, out, file_out, cubes, 1);
            return 1;
        }
        if (!has)
            break;

        if (not_first) {
            if (fputc('\n', out) == EOF) {
                fprintf(stderr, "AODjustify ERROR> Erreur lors de l'écriture du fichier de sortie\n");
                freeALL(ps, out, file_out, cubes, 1);
                return 1;
            }
        } else {
            not_first = 1;
        }

        PG pg = {0};
        int err = 0;
        if (!getPG(ps, &pg, &err)) { // double passe → P/W/n

            // Test des erreurs spécifiques
            if (err == 1) {
                fprintf(stderr, "AODjustify ERROR> Le fichier possède un mot de longueur supérieure à %d caractères : justification sur %d caractères impossible\n", Mu, Mu);
            }

            freeALL(ps, out, file_out, cubes, 1);
            return 1;
        }

        /*******************************************/
        /*       Étape 4. Justification (DP)       */
        /*******************************************/

        // Valeur d'un paragraphe (On pose LLONG_MAX en cas d'erreur interne)
        errno = 0;
        long long val = setPG(&pg, Mu, out, cubes);
        if (val == LLONG_MAX) {
            fprintf(stderr, "AODjustify ERROR> Erreur lors de la justification du texte : %s (%d)\n", strerror(errno), errno);
            freeALL(ps, out, file_out, cubes, 1);
            freePG(&pg);
            return 1;
        }
        badness += val;
        freePG(&pg);
    }

    freeALL(ps, out, file_out, cubes, 0);
    fprintf(stderr, "AODjustify CORRECT> %llu\n", badness);
    // printf("DEBUG: Total Pruned states: %lld\n", pruned);
    return 0;
}

/**
 * @brief Libère toutes les ressources détenues par le *main*.
 *
 * @param ps            Contexte de parsing (peut être NULL).
 * @param out           Flux de sortie (peut être NULL).
 * @param file_out      Chemin du fichier de sortie (peut être NULL).
 * @param cubes         Tableau des cubes pré-calculés (peut être NULL).
 * @param remove_file   Si non nul, supprime le fichier de sortie via `remove(file_out)`.
 *
 * @details
 * - Appelle `psclose(ps)` (munmap + close(fd) + free).
 * - Appelle `fclose(out)`.
 * - `free(cubes)`.
 * - Appelle `remove(file_out)` pour supprimer le fichier de sortie.
 * - Appelle `debuff()` pour libérer le tampon interne d’écriture (voir @ref out.h).
 *
 * @note Tolère des pointeurs NULL (idempotent sur chaque ressource).
 */
void freeALL(PS *ps, FILE *out, char *file_out, void *cubes, int remove_file) {
    if (ps) {
        psclose(ps);
    }
    if (out) {
        fclose(out);
    }
    if (file_out && remove_file) {
        remove(file_out);
    }
    if (cubes) {
        free(cubes);
    }
    debuff();
}