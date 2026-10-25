/**
 * @file common.h
 * @brief Constantes, catégories de caractères et utilitaires communs.
 *
 * Ce fichier déclare :
 * - la taille standard de certains buffers (@ref BUFFER_SIZE),
 * - la liste des jeux de caractères acceptés (@ref VALID_CHARSETS),
 * - la table de classification des séparateurs @ref SEP (indexée par octet ISO-8859-1),
 * - les catégories de séparateurs (masques de bits) et les macros de test (@ref is_SPACE, @ref is_EOL, @ref is_CR, @ref is_CRLF, @ref is_SEP).
 *
 * @note @ref SEP est indexée par **octet** (ISO-8859-1). Les macros castent en (unsigned char)
 *       pour éviter les UB liées à la signature de `char`.
 */
#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * @defgroup common Common (séparateurs & utilitaires)
 * @brief Aides globales (classification des séparateurs, constantes).
 * @{
 */

/** @brief Taille standard pour de petits buffers (noms de fichiers, messages, etc.). */
#define BUFFER_SIZE 256

/** @brief Taille standard pour un tableau de caractères ASCII */
#define ASCII_SIZE 256

/* Réf. : https://fr.wikipedia.org/wiki/ISO/CEI_8859-1 */
/**
 * @brief Jeux de caractères acceptés par la vérification d’entrée.
 * @details Définis et initialisés dans `check.c`. Tableau **terminé par NULL**.
 * @code
 * char *VALID_CHARSETS[] = { "iso-8859-1", "us-ascii", NULL };
 * @endcode
 *
 */
extern char *VALID_CHARSETS[]; /* check.c */

/**
 * @brief Déclarations anticipées des structures publiques (définies dans @ref parse.h).
 */
typedef struct PS PS; /**< Contexte mmap + fenêtre de paragraphe. */
typedef struct PG PG; /**< Paragraphe (tableaux P/W parallèles + n). */

/**
 * @name Catégories de caractères (masques)
 * @details Bits utilisés dans @ref SEP pour marquer les séparateurs.
 * @{
 */
enum {
    SPACE = 1u,       /**< Espace ou tabulation (`' '`, `'\t'`). */
    EOL = 2u,         /**< Fin de ligne LF (`'\n'`). */
    CR = 4u,          /**< Retour chariot CR (`'\r'`). */
    CRLF = (EOL | CR) /**< Tout retour de ligne (CR ou LF). */
};
/** @} */

/**
 * @brief Table de classification des séparateurs (indexée par octet).
 * @details
 * - Taille : 256 entrées.
 * - Convention : **0** = non-séparateur ; combinaison de bits (cf. enum) = séparateur.
 *
 * Exemple (dans `common.c`) :
 * @code
 * unsigned char SEP[BUFFER_SIZE] = {
 *   [0x09] = SPACE, // '\t'
 *   [0x20] = SPACE, // ' '
 *   [0x0A] = EOL,   // '\n'
 *   [0x0D] = CR     // '\r'
 *   // le reste = 0
 * };
 * @endcode
 *
 * @warning Table statique de classification ; ne pas modifier à l’exécution.
 */
extern unsigned char SEP[ASCII_SIZE];

/* --------------------------------------------------------------------------
 * Macros de test — cast explicite en (unsigned char) avant indexation.
 * --------------------------------------------------------------------------*/

/** @brief Vrai si @p c est un **espace** ou une **tabulation**. */
#define is_SPACE(c) (SEP[(unsigned char)(c)] & SPACE)

/** @brief Vrai si @p c est une fin de ligne **LF**. */
#define is_EOL(c) (SEP[(unsigned char)(c)] & EOL)

/** @brief Vrai si @p c est un retour chariot **CR**. */
#define is_CR(c) (SEP[(unsigned char)(c)] & CR)

/** @brief Vrai si @p c est un retour de ligne (**CR** ou **LF**). */
#define is_CRLF(c) (SEP[(unsigned char)(c)] & CRLF)

/**
 * @brief Vrai si @p c est un **séparateur** quelconque (ES/TAB/CR/LF).
 * @note Test “non-zéro” conforme à la convention de @ref SEP.
 */
#define is_SEP(c) (SEP[(unsigned char)(c)] != 0)

/** @brief Vrai si @p c est un caractère de **mot** (non-séparateur). */
#define is_NONSEP(c) (!is_SEP(c))

/** Libère toutes les ressources allouées dans le @ref main */
void freeALL(PS *ps, FILE *out, char *file_out, void *cubes, int remove_file);

#endif /* __COMMON_H */
