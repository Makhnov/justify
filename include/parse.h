#ifndef __PARSE_H
#define __PARSE_H

#include "common.h"

/**
 * @file parse.h
 * @brief Parsing « mmap » du fichier d’entrée en paragraphes et mots.
 *
 * Ce module parcourt la zone mappée par @ref PS, découpe le texte en paragraphes
 * (au sens du sujet : deux fins de ligne successives, espaces éventuels ignorés),
 * puis construit pour chaque paragraphe un tableau des mots.
 *
 * @note Les pointeurs de mots @ref PG::P pointent **dans la zone mmap** portée
 * par @ref PS et deviennent invalides après @ref psclose().
 *
 * @defgroup parser Parser (mmap → paragraphes → mots)
 * @{
 */

/**
 * @brief Structure paragraphe (fenêtre logique [para_s, para_e)).
 *
 * Deux tableaux @ref PG::P et @ref PG::W **à indices parallèles** décrivent les mots :
 * - `P[i]` : début du mot *i* (pointeur dans le mmap),
 * - `W[i]` : longueur (en caractères) du mot *i*.
 */
struct PG {
    long long n;         ///< Nombre de mots du paragraphe.
    unsigned char **P;   ///< Pointeurs vers le début de chaque mot (dans le mmap).
    uint16_t *W;         ///< Longueurs de mots (≤ M).
    unsigned char *base; ///< Base du mmap global (copie de PS::base pour commodité).
};

/**
 * @brief Contexte de parsing : zone mmap + fenêtre courante de paragraphe.
 *
 * - `fd` : descripteur du fichier mappé (lecture ; fermé par @ref psclose()).
 * - `M`  : largeur de ligne (Mu) utilisée pour **refuser** un mot `> M`.
 * - `base..end` : délimitation de la zone mappée ; `cur` : curseur global.
 * - `para_s..para_e` : fenêtre du paragraphe courant (mise à jour par @ref nextPG()).
 */
struct PS {
    int fd;
    int M;
    size_t size;

    unsigned char *base;
    unsigned char *cur;
    unsigned char *end;

    unsigned char *para_s;
    unsigned char *para_e;
};

/**
 * @brief Ouvre un contexte de parsing sur un fichier déjà ouvert et mappé en lecture.
 *
 * @param fd   Descripteur de fichier ouvert en lecture (sera fermé par @ref psclose()).
 * @param size Taille totale du fichier (octets).
 * @param M    Largeur de ligne (Mu). Mot de longueur strictement supérieure → erreur.
 *
 * @return Pointeur valide sur @ref PS en cas de succès, `NULL` sinon.
 *
 * @pre `fd >= 0`, `size > 0`.
 * @post `ps->base` non nul ; `ps->end = ps->base + size` ; `ps->cur = ps->base`.
 */
PS *psopen(int fd, size_t size, int M);

/**
 * @brief Cherche et positionne la **fenêtre [para_s, para_e)** du prochain paragraphe.
 *
 * @note On supprime volontairement les espaces en début et fin de paragraphe.
 *
 * @param ps Contexte de parsing.
 * @retval  1 Paragraphe trouvé ; `para_s..para_e` positionnés.
 * @retval  0 Fin de fichier (plus de paragraphe).
 * @retval -1 Erreur (ex. `ps == NULL`).
 */
int nextPG(PS *ps);

/**
 * @brief Construit la description des mots du paragraphe courant.
 *
 * @param ps     Contexte (fenêtre déjà positionnée par @ref nextPG()).
 * @param out    Sortie paragraphe (champs `P`/`W` **alloués ici**).
 * @param p_err  Optionnel : si non-NULL, **mis à 1 uniquement** si un mot de longueur `> M`
 *               est rencontré ; sinon **non modifié**. (Actuellement, pas d’autre code d’erreur.)
 *
 * @retval 1 Succès (out->n, out->P, out->W, out->base remplis).
 * @retval 0 Échec (allocation ou mot `> M`).
 *
 * @pre `ps != NULL`, `out != NULL`.
 * @post En cas d’échec, les allocations partielles sont libérées (pas de fuite).
 */
int getPG(PS *ps, PG *out, int *p_err);

/**
 * @brief Libère les ressources détenues par un @ref PG.
 * @param pg Paragraphe à libérer (champs remis à zéro).
 */
void freePG(PG *pg);

/**
 * @brief Ferme le contexte, démonte le mmap et **ferme le descripteur**.
 * @param ps Contexte à détruire (tolère `NULL`).
 *
 * @post Après retour, tous les pointeurs vers le mmap (ex. `PG::P[i]`) sont invalides.
 */
void psclose(PS *ps);

#endif /* __PARSE_H */
