/**
 * @file parse.c
 * @brief Implémentations du module de parsing, ouverture du fichier et délimitation des paragraphes.
 * @see parse.h pour des informations plus détaillées sur les structures et fonctions publiques.
 */

#include "parse.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

/** @brief Consomme un CR, un LF ou une paire CRLF à `*p` (si présents) et avance le pointeur.
 *  @param p   [in,out] Curseur dans le mmap.
 *  @param end [in]     Fin de la zone mmap.
 *  @retval 1 Au moins un caractère de fin de ligne consommé.
 *  @retval 0 Aucun caractère de fin de ligne à consommer.
 *
 *  @note Interne au module ; utilisée par @ref nextPG() pour reconnaître CR/LF/CRLF.
 */
static inline int eat_cr(unsigned char **p, unsigned char *end) {
    // On vérifie si on est hors limites
    if (*p >= end)
        return 0;
    // Si on a un CR
    if (is_CR(**p)) {
        // On mange le CR
        (*p)++;
        if (*p >= end) {
            return 1;
        }
        // Si on a un LF juste après (CRLF)
        if (is_EOL(**p)) {
            // On mange le LF
            (*p)++;
        }
        // On a mangé un CR ou un CRLF
        return 1;
    }

    // Si on a un LF seul
    if (is_EOL(**p)) {
        // On mange le LF
        (*p)++;
        return 1;
    }

    // Dans tous les autres cas, on ne mange rien
    return 0;
}

/** @brief Voir @ref psopen() (contrat détaillé). Ouvre et mmap le fichier en mémoire, initialise le parser principal @ref PS. */
PS *psopen(int fd, size_t s, int M) {
    unsigned char *base = mmap(NULL, s, PROT_READ, MAP_PRIVATE, fd, 0);
    if (base == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    // On alloue notre zone mémoire pour le parser
    PS *ps = (PS *)calloc(1, sizeof(PS));
    if (!ps) {
        if (base) {
            munmap((void *)base, s);
        }
        close(fd);
        return NULL;
    }

    ps->fd = fd;          // File descriptor
    ps->base = base;      // Pointeur vers le début du mmap
    ps->cur = base;       // Pointeur courant
    ps->end = base + s;   // Fin de la zone mappée
    ps->size = s;         // Espace mémoire
    ps->M = M;            // Mu (μ dans l'énoncé)
    ps->para_s = ps->cur; // Début du paragraphe
    ps->para_e = ps->cur; // Fin du paragraphe

    return ps;
}

/** @brief Voir @ref nextPG() : détecte deux retours (CR/LF/CRLF) séparés ou non par espaces pour borner [para_s, para_e). */
int nextPG(PS *ps) {

    /* La fin d'un paragraphe est ainsi définie : (\r\n|\r|\n)  [ \t|' ']* (\r\n|\r|\n), càd, un saut de ligne, zéro ou plusieurs espaces, et un autre saut de ligne. */
    if (!ps) {
        return -1;
    }
    unsigned char *p, *end;
    p = ps->cur;
    end = ps->end;

    /* ON SUPPRIME LES ESPACES EN DÉBUT (ET EN FIN, cf. return ci-dessous) DE PARAGRAPHE */

    while (p < end) {
        // On saute directement les espaces
        if (is_SPACE(*p)) {
            p++;
            // Pour les CR/LF/CRLF on utilise la fonction dédiée
        } else if (is_CRLF(*p)) {
            eat_cr(&p, end);
        } else {
            // Début d'un mot
            break;
        }
    }

    if (p >= end) {
        // On met à jour la position courante
        ps->cur = p;
        return 0;
    }

    // On marque le "vrai" début du paragraphe
    unsigned char *start = p;

    /* ON CHERCHE LA FIN (DEUX CR|LF CONSÉCUTIFS) DU PARAGRAPHE */

    // Booléen qui indique la présence d'un CR
    int crlf = 0;

    // On scanne le texte
    while (p < end) {
        // On n'a encore aucun saut de ligne
        if (!crlf) {
            // Quand on en trouve un on le mange et on active le booléen
            if (is_CRLF(*p)) {
                eat_cr(&p, end);
                crlf = 1;
                continue;
            }
            // Pas de CR, on continue
            p++;
        } else {
            // on a vu un CR, tant qu'on a des espaces on peut les ignorer
            while (p < end && is_SPACE(*p)) {
                p++;
            }
            // Si on est arrivé à la fin du fichier
            if (p >= end) {
                // Par souci de cohérence
                crlf = 0;
                break;
            }
            // si le prochain caractère après 0 ou n espaces est un CR on a trouvé une fin de paragraphe
            if (is_CRLF(*p)) {
                eat_cr(&p, end);
                break;
            } else {
                // Sinon c'était simplement un retour à la ligne dans un paragraphe, on reset le booléen
                crlf = 0;
            }
        }
    }

    ps->para_s = start;
    ps->para_e = p;

    // On se positionne après le paragraphe pour la prochaine lecture
    ps->cur = p;
    return 1;
}

/** @brief Voir @ref getPG() : remplit P et W en parcourant de para_s à para_e. Renvoie 1 et mets p_err à 1 si trouve un mot > M. */
int getPG(PS *ps, PG *out, int *p_err) {
    if (!ps || !out)
        return 0;

    unsigned char *s = ps->para_s;
    unsigned char *e = ps->para_e;
    size_t len = (size_t)(e - s);

    // Maximum de mots théorique : (taille du paragraphe + 1) / 2 (1 caractère + 1 séparateur)
    size_t max = (len + 1) / 2 + 1;

    uint16_t *W = (uint16_t *)malloc(max * sizeof(*W));
    unsigned char **P = (unsigned char **)malloc(max * sizeof(*P));
    if (!P || !W) {
        free(P);
        free(W);
        return 0;
    }

    long long n = 0;
    int in_word = 0;
    unsigned char *wstart = NULL;
    long long l = 0;

    unsigned char *p = s;
    while (p < e) {
        unsigned char c = *p;

        // Fin éventuelle de mot
        if (is_SEP(c)) {

            // On est déjà dans un mot
            if (in_word) {

                // On a dépassé la longueur maximale autorisée
                if (l > (long long)ps->M) {
                    if (p_err) {
                        *p_err = 1;
                    }
                    free(P);
                    free(W);
                    return 0;
                }
                P[n] = wstart;
                W[n] = (uint16_t)l;
                n++;
                in_word = 0;
                l = 0;
            }
            p++;
            continue;
        }

        // On n'est pas dans un mot
        if (!in_word) {
            in_word = 1;
            wstart = p;
            l = 1;
        } else {
            l++;
        }
        p++;
    }

    // mot final éventuel
    if (in_word) {
        if (l > (long long)ps->M) {
            if (p_err) {
                *p_err = 1;
            }
            free(P);
            free(W);
            return 0;
        }
        P[n] = wstart;
        W[n] = (uint16_t)l;
        n++;
    }

    out->n = n;
    out->base = ps->base;
    out->P = P;
    out->W = W;
    return 1;
}

/** @brief Voir @ref freePG() : libère P/W et réinitialise la structure à zéro. */
void freePG(PG *pg) {
    if (!pg)
        return;
    free(pg->P);
    pg->P = NULL;
    free(pg->W);
    pg->W = NULL;
    pg->n = 0;
    pg->base = NULL;
}

/** @brief Voir @ref psclose() : munmap, close(fd) et free. Les pointeurs vers le mmap deviennent invalides. */
void psclose(PS *ps) {
    if (!ps) {
        return;
    }
    if (ps->base && ps->size) {
        munmap(ps->base, ps->size);
    }
    if (ps->fd >= 0) {
        close(ps->fd);
    }
    free(ps);
}

/** @brief Voir @ref SEP : tableau de délimiteurs pour le parsing. */
unsigned char SEP[ASCII_SIZE] = {
    [0x09] = SPACE, // \t
    [0x20] = SPACE, // ' '
    [0x0A] = EOL,   // \n
    [0x0D] = CR     // \r
};