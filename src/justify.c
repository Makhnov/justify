#include "justify.h"
#include <errno.h>
#include <limits.h>

/**
 * @file justify.c
 * @brief Module qui implémente l'algo principal de la DP. Nous avons choisi de le coder en itératif.
 *
 * @details
 * @par Implémentation :
 * - allocation des tableaux J (coûts minimaux) et N (index de reconstruction) ;
 * - boucle principale descendante sur i (dernier mot vers le premier) qui cherche le meilleur j tel que le rab ugly + J[j+1] est minimal ;
 * ugly est égal à M moins la somme des mots, il est décrémenté à chaque itération de j (sauf la première) ;
 * pour simuler un espace entre les mots On conserve le minimum ainsi que l'index j correspondant dans best et bestj.
 *
 * @par Optimisation :
 * - on limite j à jmax, la borne supérieure du meilleur j trouvé à l'itération précédente (on évite ainsi des itérations inutiles)
 * - on utilise une variable locale ugly pour éviter de recalculer M - width à chaque itération
 * - o̶n̶ ̶a̶p̶p̶l̶i̶q̶u̶e̶ ̶u̶n̶ ̶p̶r̶u̶n̶i̶n̶g̶ ̶s̶i̶m̶p̶l̶e̶ ̶:̶ ̶s̶i̶ ̶J̶[̶j̶+̶1̶]̶ ̶>̶=̶ ̶b̶e̶s̶t̶,̶ ̶o̶n̶ ̶a̶r̶r̶ê̶t̶e̶ ̶l̶a̶ ̶r̶e̶c̶h̶e̶r̶c̶h̶e̶ ̶p̶o̶u̶r̶ ̶c̶e̶t̶ ̶i̶ ̶(̶a̶u̶c̶u̶n̶ ̶j̶ ̶p̶l̶u̶s̶ ̶g̶r̶a̶n̶d̶ ̶n̶e̶ ̶p̶e̶u̶t̶ ̶ê̶t̶r̶e̶ ̶m̶e̶i̶l̶l̶e̶u̶r̶)̶
 *
 * @par Reconstruction et écriture :
 *  On part de i=0, j = N[i]-1, et on applique i = j+1 jusqu'à la fin du paragraphe. A chaque étape on écrit la ligne i..j via @ref writeLN (détaillée dans @ref out.h).
 *
 * @note Le tableau `cubes` est indexable sur 0..M (nous l'avons pré-calculé dans le @ref main).
 * @see justify.h pour des informations plus détaillées sur les structures et fonctions publiques.
 */

/** @brief Voir @ref setPG (contrat détaillé dans justify.h). */
long long setPG(PG *pg, int M, FILE *out, long long *cubes) {
    if (!pg || !out) {
        errno = EINVAL;
        return LLONG_MAX;
    }

    long long n = pg->n;

    long long *J, *N;
    uint16_t *W = pg->W;

    /* Impossible dans le flux attendu, cf. nextPG() */
    // if (!pg->n)
    //     return 0;

    J = (long long *)malloc((size_t)(n + 1) * sizeof(long long));
    N = (long long *)malloc((size_t)n * sizeof(long long));

    if (!J || !N) {
        free(J);
        free(N);
        errno = ENOMEM;
        return LLONG_MAX;
    }

    J[n] = 0;

    long long jmax = n - 1; // borne initiale pour j

    /* HOT SPOT */
    for (int32_t i = (int32_t)n - 1; i >= 0; --i) {

        long long best = LLONG_MAX - 1;
        long long bestj = (long long)i;
        int32_t ugly = M;

        // On limite j à jmax connu
        for (long long j = (long long)i; j <= jmax; ++j, ugly--) {
            ugly -= W[j];
            if (ugly < 0) {
                break;
            }
            // Optimisation #1 : Pruning, si le coût de J[j+1] est déjà pareil ou moins bien (>=) que le best actuel, J[j+1] n'est pas candidat
            // if (J[j + 1] >= best) {
            //     // pruned++;
            //     break;
            // }

            // uint32_t cost = (j == n - 1) ? 0 : cubes[(uint32_t)(M - width)];
            // uint32_t cand = cost + J[j + 1];
            long long cand = ((j == n - 1) ? 0 : cubes[(long long)(ugly)]) + J[j + 1];

            if (cand < best) {
                best = cand;
                bestj = j;
            }
        }

        J[i] = best;
        N[i] = bestj + 1;

        // Optimisation #2 : On peut réduire jmax pour les itérations à venir, on évite environ n/2 itérations inutiles
        jmax = bestj;
    }

    // Reconstruction + écriture à la volée
    long long i = 0;
    while (i < n) {
        long long j = N[i] - 1;
        if (!writeLN(pg, i, j, M, out)) {
            free(J);
            free(N);
            errno = EIO;
            return LLONG_MAX;
        }
        i = j + 1;
    }

    long long badness = J[0];
    free(J);
    free(N);
    return badness;
}