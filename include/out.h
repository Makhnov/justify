/**
 * @file out.h
 * @brief Écriture d'une ligne justifiée dans un flux `FILE*`.
 *
 * @details
 * Ce module fournit la primitive publique @ref writeLN pour écrire une **ligne**
 * du paragraphe (mots `i..j`) justifiée sur la largeur `M`, avec répartition des
 * espaces « au plus régulier » :
 *
 * - on place **au moins un espace** entre chaque paire de mots ;
 * - on répartit le « rab » `(M - somme_longueurs - nb_gaps)` en ajoutant
 *   `⌊rab / gaps⌋` espaces à chaque inter-mot, puis **1 espace supplémentaire**
 *   aux `rab % gaps` premiers inter-mots ;
 * - la **dernière ligne** (`j == n - 1`) n’est **pas** forcée à `M` : on écrit des
 *   espaces simples entre les mots et on termine par `'\n'`.
 *
 * L’implémentation utilise un **tampon interne** réutilisable (dans `out.c`)
 * pour limiter les I/O fragmentées (`fwrite`).
 */
#ifndef __OUT_H
#define __OUT_H

#include "parse.h"

/**
 * @brief Écrit les mots `i..j` du paragraphe @p pg justifiés sur la largeur @p M.
 *
 * @param pg   Paragraphe (tableaux @ref PG::P et @ref PG::W).
 * @param i    Indice du **premier** mot de la ligne (inclus).
 * @param j    Indice du **dernier** mot de la ligne (inclus).
 * @param M    Largeur cible (nombre de colonnes).
 * @param out  Flux de sortie (ouvert en écriture).
 *
 * @pre
 * - `pg != NULL`, `out != NULL`.
 * - `0 <= i <= j < pg->n`.
 * - La ligne tient dans `M` **selon le DP en amont** (l’appelant garantit la validité).
 *
 * @post
 * - Écrit **exactement** `j - i` blocs d’espaces entre les mots.
 * - Termine la ligne par un **`'\n'`** unique.
 *
 * @return 1 en cas de succès ; 0 en cas d’erreur (allocation interne ou écriture).
 *
 * @note
 * - Cas « un seul mot » (`i == j`) : le mot est écrit suivi d’un `'\n'` (pas de remplissage à `M`).
 * - Les octets de mots sont recopiés tels quels depuis la zone mmap (encodage source).
 * - La **dernière ligne** (`j == n - 1`) n’est **pas nécessairement** remplie jusqu’à `M`.
 *
 * @warning
 * - Ici on fait confiance à l’appelant pour la validité des indices i et j (0 <= i <= j < n).
 * - On suppose aussi que i et j sont bien contenus dans la largeur `M` (@ref sumLen + gaps <= M).
 *
 */
int writeLN(PG *pg, long long i, long long j, int M, FILE *out);

/**
 * @brief Libère le buffer interne utilisé par @ref writeLN et réinitialise l'état.
 *
 * @post Après appel, `buffer == NULL` et `buff_size == 0`.
 * @note Appelée en fin de programme (cf. `freeALL`) pour éviter les fuites détectées
 *       par Valgrind (« still reachable »).
 */
void debuff();

#endif /* __OUT_H */
