/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: strlib/hash.c
 *
 * Description: Hash Tables
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


/*****************************************************************************
 *
 * Headers
 *
 */

/* System headers */
#include <stdlib.h> /* malloc(), free(), NULL */
#include <string.h> /* memcpy(), strcmp()     */
#include <assert.h> /* assert()               */

/* Project headers */
#include <common.h>
#include "hash.h"


/*****************************************************************************
 *
 * Private functions
 *
 */

/* Hash Algorithm Explanation

   The hash key is 10 bits long (so the table size is 4 KB on 32-bit
   machines) and is composed as this:

	     +---+---+---+---+---+---+---+---+---+---+
   Bits   -> | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
	     +---+---+---+---+---+---+---+---+---+---+
   Fields -> |Length |   Sum of rotated characters   |
	     +-------+-------------------------------+

   Length: string length % 4 (the 2 least significant bits).
   Sum: each character is left-rotated with a byte boundary (bits that get
	out of the byte are reinserted to the right) then summed up; the
	result is the least significant byte of the sum. */

/* Left-rotate a number of given length by a specified offset */
#define ROT_LEFT(x, offset, bits) \
    (((offset) % (bits)) == 0 ? ((x) & ((1 << (bits)) - 1)) : (((x) << \
    ((offset) % (bits))) & ((1 << (bits)) - 1)) | (((x) & ((1 << (bits)) - \
    1)) >> ((bits) - ((offset) % (bits)))))

/*
 * Hash function.
 */
static int hash_function(const char *const str)
{
    int i;
    unsigned chr;
    unsigned sum;

    assert(str != NULL);

    /* See the explanation above */
    sum = 0;
    for (i = 0; (chr = (unsigned char) str[i]) != '\0'; i++)
	sum += ROT_LEFT(chr, i, 8);

    return ((i & 3) << 8) | (sum & 0xFF);
}


/*****************************************************************************
 *
 * Public functions
 *
 */

/*
 * Create a new hash table.
 */
hash_t *hash_new(void)
{
    hash_t *hash;

    if ((hash = malloc(sizeof(hash_t))) == NULL)
	hash_init(hash);
    return hash;
}

/*
 * Delete a hash table.
 */
void hash_delete(hash_t *const hash)
{
    assert(hash != NULL);

    hash_free(hash);
    free(hash);
}

/*
 * Initialize a hash table.
 */
void hash_init(hash_t *const hash)
{
    assert(hash != NULL);

    memset(hash->table, 0, sizeof(hash->table));
}

/*
 * Free a hash table.
 */
void hash_free(const hash_t *const hash)
{
    int             key;
    hash_element_t *element;

    assert(hash != NULL);

    /* Free all malloc'ed elements */
    for (key = 0; key < HASH_SIZE; key++)
	if ((element = hash->table[key]) != NULL) {
	    while (element->next != NULL) {
		element = element->next;
		if (element->prev->alloced == 1)
		    free(element->prev);
	    }

	    if (element->alloced == 1)
		free(element);
	}
}

/*
 * Add an element to the hash table.
 */
hash_element_t *hash_add(hash_t *const hash, const char *const str,
			 const void *const object, hash_element_t *element)
{
    int key;

    assert(hash != NULL);
    assert(str != NULL);

    /* Allocate memory if necessary */
    if (element != NULL)
	element->alloced = 0;
    else {
	if ((element = malloc(sizeof(hash_element_t))) == NULL)
	    return NULL;
	element->alloced = 1;
    }

    /* Get the key */
    key = hash_function(str);
    assert(key >= HASH_MIN && key <= HASH_MAX);
    key -= HASH_MIN;

    /* Initialise element */
    element->object = object;
    element->str = str;
    element->key = key;
    element->next = hash->table[key];
    element->prev = NULL;

    /* Link the element to the list */
    if (element->next != NULL)
	hash->table[key]->prev = element;
    hash->table[key] = element;

    return element;
}

/*
 * Remove an element from the hash table.
 */
void *hash_remove(hash_t *const hash, hash_element_t *const element)
{
    const void *object;

    assert(hash != NULL);
    assert(element != NULL);
    assert(element->key >= 0 && element->key < HASH_SIZE);

    object = element->object;

    /* Unlink element from the list */
    if (element->prev != NULL)
	element->prev->next = element->next;
    else
	hash->table[element->key] = element->next;
    if (element->next != NULL)
	element->next->prev = element->prev;

    if (element->alloced == 1)
	free(element);

    return (void *) object;
}

/*
 * Find an element in the hash table.
 */
hash_element_t *hash_find(const hash_t *const hash, const char *const str)
{
    int             key;
    hash_element_t *element;

    assert(hash != NULL);

    /* Get the key */
    key = hash_function(str);
    assert(key >= HASH_MIN && key <= HASH_MAX);
    key -= HASH_MIN;

    /* Walk through the linked list */
    for (element = hash->table[key]; element != NULL; element = element->next)
	if (strcmp(str, element->str) == 0)
	    break;

    return element;
}

/* End of file */
