/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: strlib/hash.h
 *
 * Description: Hash Tables (Header)
 *
 * ---------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ---------------------------------------------------------------------------
 */


#ifndef HASH_H
#define HASH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*
 * Constants
 */

#define HASH_BITS 10                        /* Hash key length in bits  */
#define HASH_MIN  0                         /* Minimum value of the key */
#define HASH_MAX  ((1 << HASH_BITS) - 1)    /* Maximum value of the key */
#define HASH_SIZE (HASH_MAX - HASH_MIN + 1) /* Number of key values     */


/*
 * Data types
 */

/* Fash table element */
typedef struct hash_element {
    int                  alloced; /* If element has been auto-malloc'ed  */
    const void          *object;  /* Pointer to the corresponding object */
    const char          *str;     /* Pointer to the corresponding string */
    int                  key;     /* Hash key                            */
    struct hash_element *next;    /* Next element with the same key      */
    struct hash_element *prev;    /* Previous element with the same key  */
} hash_element_t;

/* Hash table */
typedef struct hash {
    hash_element_t *table[HASH_SIZE]; /* Pointer the elements for each key */
} hash_t;


/*
 * Prototypes
 */

/* Constructors and destructors */
hash_t *hash_new(void);
void    hash_delete(hash_t *const hash);
void    hash_init(hash_t *const hash);
void    hash_free(const hash_t *const hash);

/* Methods */
hash_element_t *hash_add(hash_t *const hash, const char *const str,
			 const void *const object, hash_element_t *element);
void           *hash_remove(hash_t *const hash,
			    hash_element_t *const element);
hash_element_t *hash_find(const hash_t *const hash, const char *const str);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !HASH_H */

/* End of file */
