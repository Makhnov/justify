#ifndef __JUSTIFY_H
#define __JUSTIFY_H

#include "out.h"

/**
 * @file justify.h
 * @brief Programmation dynamique d'un paragraphe + écriture des lignes.
 *
 * @details
 * Cette unité fournit l'implémentation DP dite « itérative » sur un paragraphe déjà
 * tokenisé (voir @ref PG). Le calcul du coût minimal utilise deux tableaux indexés de manière parallèle :
 * - `J[i]` : coût minimal du sous-paragraphe commençant au mot `i`,
 * - `N[i]` : index du premier mot de la ligne suivante (pour la reconstruction).
 *
 * La reconstruction s'effectue à la volée, directement après la DP en appelant @ref writeLN()
 * pour chaque ligne déterminée (écriture dans le `FILE*` passé en argument). La norme renvoyée
 * est `J[0]`, i.e. par `norme`, on entend la somme des cubes du reliquat des lignes **internes** uniquement
 * (on ne compte pas la dernière ligne), conformément au sujet.
 *
 * @note L’écriture de chaque ligne est déléguée à @ref writeLN (out.h / out.c).
 *       Le tableau `cubes` contient les valeurs `i^3` pour `0 ≤ i ≤ M`.
 */

/**
 * @brief Application algorithmique des principes de la programmation dynamique et écriture d’un paragraphe.
 *
 * @param pg     Paragraphe (mots et longueurs) déjà construit (voir parse.h).
 * @param M      Largeur de ligne (Mu). Sert de borne et pour l’indexation de `cubes`.
 * @param out    Flux de sortie ouvert en écriture binaire/texte.
 * @param cubes  Tableau pré-calculé tel que `cubes[x] = x^3` pour tout `0 ≤ x ≤ M`.
 *
 * @return La norme (long long) du paragraphe justifié (coût minimal). Retourne `0`
 *         si `pg==NULL`, `out==NULL`, `pg->n==0` ou en cas d’échec d’allocation.
 *
 * @pre  `pg != NULL`, `out != NULL`, `M ≥ 0`, `pg->n ≥ 0`,
 *       `cubes` indexable sur `0..M`.
 * @post Le paragraphe est **écrit** dans `out` (une ligne par appel à @ref writeLN).
 *
 * @warning En cas d’échec d’allocation interne, la fonction renvoie `LLONG_MAX` (même type que le « vrai » coût).
 *         L’appelant doit vérifier le retour et, le cas échéant, afficher un message d'erreur en utilisant `errno` (cf. @ref main.c).
 *
 * @note Complexité :
 *       - Temps : O(n · L) où L est le nombre max de mots qui tiennent sur une ligne
 *         (depuis une position i), i.e. le plus grand L tel que Σ W[i..i+L-1] + (L-1) ≤ M.
 *         Pour un M fixé (cas de ce programme), L est borné par une constante ⇒ O(n).
 *       - Mémoire : O(n) pour les tableaux J et N.
 */
long long setPG(PG *pg, int M, FILE *out, long long *cubes); // DP + écriture, retourne la norme du paragraphe

#endif /* __JUSTIFY_H */
