#ifndef __CHECK_H
#define __CHECK_H

#include "common.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/**
 * @file check.h
 * @brief Vérifications sur le fichier d'entrée : taille du fichier, format, encodage, etc.
 *
 * @details
 * - @ref getSize : récupère la **taille** via `fstat()`. En cas d’échec ou taille nulle,
 *                  **écrit un message sur stderr et ferme fd**.
 * - @ref fileBI  : vérifie le **charset** avec `file -bi -- <path>` :
 *                  redirection de la **STDOUT** du fils vers une pipe et de la **STDERR** vers une autre (erreurs `dup2`/`execlp`/`file`).
 *
 * @note @ref VALID_CHARSETS (définie dans check.c) contient la liste blanche (ex. "iso-8859-1", "us-ascii").
 */

/**
 * @brief Retourne la taille (octets) du fichier pointé par @p fd.
 *
 * @param fd       Descripteur de fichier **ouvert**.
 * @param filename Nom du fichier (uniquement pour les messages d’erreur).
 *
 * @return Taille en octets si succès, `0` sinon.
 *
 * @post Si échec (`fstat`), **ou** si la taille vaut 0, la fonction **affiche un message sur stderr
 *       et ferme @p fd**, puis retourne 0. En cas de succès, @p fd reste **ouvert**.
 */
size_t getSize(int fd, char *filename);

/**
 * @brief Vérifie l'encodage via `file -bi -- <path>` (liste blanche @ref VALID_CHARSETS).
 *
 * @param path    Chemin du fichier (non NULL).
 * @param buffer  Tampon de sortie :
 *                - **succès** : contient la **ligne brute** renvoyée par `file -bi`.
 *                - **échec**  : contient un **message explicite** (échec pipe/fork/dup2/exec/wait,
 *                  non-texte, inode non régulier, charset refusé, etc.).
 * @param s       Capacité du tampon @p buffer (octets, **incluant** le `'\0'`).
 *
 * @retval 1 Le fichier est de type `text` et son charset est valide (présent dans @ref VALID_CHARSETS).
 * @retval 0 Erreur d’exécution **ou** charset invalide (dans ce cas, @p buffer décrit la cause du refus).
 *
 * @pre `path != NULL`, `buffer != NULL`, `s > 0`.
 *
 * @note
 * **Implémentation (résumé)** :
 *  - deux pipes : `pipefd` (capture **STDOUT** du fils) et `pipe_err` (capture **STDERR** du fils) ;
 *  - **fils** : `dup2(pipefd[1], STDOUT_FILENO)` et `dup2(pipe_err[1], STDERR_FILENO)`, puis
 *               `execlp("file","file","-bi","--",path,NULL)` ; en cas d’échec `dup2`/`execlp`,
 *               écrit le message d’erreur sur **STDERR** (redirigée) et `_exit(1)` ;
 *  - **père** : lit **STDOUT** dans @p buffer (puis **ajoute `'\0'`**), `waitpid()` ; lit **STDERR** :
 *               si un message y est présent, il **remplace** @p buffer et retourne 0 ; sinon,
 *               si `WEXITSTATUS(status) != 0` : message générique et 0 ; sinon, vérifie @ref VALID_CHARSETS
 *               (via `strstr`) ; à défaut, messages dédiés (“inode”, “pas texte”, “pas ISO-8859-1”).
 *
 * @warning En cas de succès (retour 1), le contenu de @p buffer est la **ligne `file -bi`** telle que lue par le père, l’appelant **peut l’ignorer**.
 */
int fileBI(char *path, char *buffer, size_t s);

#endif /* __CHECK_H */
