#include "out.h"

/**
 * @file out.c
 * @brief Implémentation de l’écriture d’une ligne (`writeLN`).
 *
 * Fournit l’implémentation de `writeLN` déclarée dans @ref out.h.
 * Se référer au contrat documenté dans le header pour le rôle des
 * paramètres (paragraphe @ref PG, indices `[i..j]`, largeur `M`,
 * et flux `FILE*`).
 *
 * @see justify.c, notamment la fonction @ref setPG pour voir la manière dont les couples d’indices
 *      `(i, j)` sont produits lors de la reconstruction.
 *
 * @ingroup output
 */

/** @brief Buffer interne réutilisé pour limiter les I/O fragmentées.
 * @note Dimension gérée par @ref rebuff(). */
static char *buffer = NULL;

/** @brief Taille actuelle du tampon (en octets). */
static size_t buff_size = 0;

/** @brief Libère le buffer interne utilisé pour les écritures de @ref writeLN. */
void debuff() {
    free(buffer);
    buffer = NULL;
    buff_size = 0;
}

/**
 * @brief (Interne) Garantit que la capacité du tampon interne est **≥ need**.
 *
 * @param need Capacité minimale requise (en octets).
 * @retval 1 succès (tampon prêt à l’emploi, inchangé ou réalloué).
 * @retval 0 échec (overflow de calcul ou échec de `realloc`).
 *
 * @details
 * - Politique d’agrandissement simple par doublement (x2) à partir de 256 octets.
 * - Prévention d’overflow : si `next <= new_size`, on considère un dépassement et on échoue.
 * - Cette fonction **ne renseigne pas** `errno` explicitement.
 *
 * @note Utilisation interne par @ref writeLN ; l’appelant gère le retour (0/1).
 */
static inline int rebuff(size_t need) {
    if (buff_size >= need) {
        return 1;
    }

    size_t new_size = buff_size ? buff_size : 256;
    while (new_size < need) {
        size_t next = new_size << 1;

        // Prévenir l'overflow
        if (next <= new_size) {
            return 0;
        }

        new_size = next;
    }

    void *nb = realloc(buffer, new_size);
    if (!nb) {
        return 0;
    }

    buffer = (char *)nb;
    buff_size = new_size;
    {
        return 1;
    }
}

/** @brief Voir @ref writeLN : applique l’écriture de la ligne (indices i..j). */
int writeLN(PG *pg, long long i, long long j, int M, FILE *out) {
    long long gaps = (j > i) ? (j - i) : 0;

    long long n = pg->n;
    uint16_t *W = pg->W;
    unsigned char **P = pg->P;

    // Cas "un seul mot" sur la ligne
    if (!gaps) {
        if (fwrite(P[i], 1, W[i], out) != (size_t)W[i]) {
            return 0;
        }
        if (fputc('\n', out) == EOF) {
            return 0;
        }
        return 1;
    }

    // Dernière ligne du paragraphe : espaces simples
    if (j == n - 1) {
        for (long long k = i; k <= j; ++k) {
            if (fwrite(P[k], 1, W[k], out) != (size_t)W[k]) {
                return 0;
            }
            if (k < j) {
                if (fputc(' ', out) == EOF) {
                    return 0;
                }
            }
        }
        if (fputc('\n', out) == EOF) {
            return 0;
        }
        return 1;
    }
    // Calcul de la longueur totale des mots
    long long sumLen = 0;
    for (long long k = i; k <= j; ++k) {
        sumLen += W[k];
    }

    // Ligne justifiée : répartir le rab
    int32_t rab = M - (sumLen + gaps);
    long long base = (gaps > 0) ? (long long)(rab / (int32_t)gaps) : 0;
    long long extra = (gaps > 0) ? (long long)(rab % (int32_t)gaps) : 0;

    // On va avoir besoin de exactement M caractères + '\n'
    size_t require = (size_t)M + 1;
    if (!rebuff(require)) {
        return 0;
    }
    char *p = buffer;

    for (long long k = i; k <= j; ++k) {
        memcpy(p, P[k], (size_t)W[k]);
        p += W[k];

        if (k < j) {
            // 1 espace de base + base + (répartir le reste sur les premiers 'extra' gaps)
            long long nb = 1 + base + ((k - i) < extra ? 1u : 0u);
            memset(p, ' ', nb);
            p += nb;
        }
    }
    *p++ = '\n';

    // On écrit toute la ligne d'un coup
    if (fwrite(buffer, 1, (size_t)(p - buffer), out) != (size_t)(p - buffer)) {
        return 0;
    }

    // Tout s'est bien passé
    return 1;
}