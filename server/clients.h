/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: server/clients.h
 *
 * Description: Client Manager (Header)
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


#ifndef CLIENTS_H
#define CLIENTS_H

/*
 * Headers
 */

/* System headers */
#include <sys/select.h> /* fd_set */

/* Project headers */
#include <iobuffer.h> /* iobuffer_t */
#include <hash.h>     /* hash_t     */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*
 * Data types
 */

/* Structure defining a connected client */
typedef struct client {
    struct client *next;     /* Next element in linked list      */
    struct client *prev;     /* Previous element in linked list  */
    iobuffer_t     buffer;   /* Input/output buffer              */
    char          *nick;     /* Nickname                         */
    int            nick_len; /* Nickname length                  */
    char           addr[22]; /* Client address and port (string) */
    int            addr_len; /* Address length                   */
    hash_element_t hash_elm; /* Element in hash table            */
} client_t;

/* Structure used for clients managing */
typedef struct clients {
    int         number;    /* Number of connected clients */
    client_t   *first;     /* First client in linked list */
    client_t   *last;      /* Last client in linked list  */
    fd_set     *read_fds;  /* Read descriptor set         */
    fd_set     *write_fds; /* Write descriptor set        */
    iobuffer_t *console;   /* Console I/O buffer          */
    int         srv_sock;  /* Server socket               */
    hash_t      hash;      /* Client hash table           */
} clients_t;


/*
 * Prototypes
 */

/* Constructors and destructors */
void clients_init(clients_t *const clients, fd_set *const read_fds,
		  fd_set *const write_fds, struct iobuffer *const console,
		  const int srv_sock);
void clients_free(clients_t *const clients);

/* Methods */
int       clients_add(clients_t *const clients);
void      clients_remove(clients_t *const clients, client_t *const client);
void      clients_disconnect(client_t *const client);
int       clients_read(clients_t *const clients);
int       clients_write(clients_t *const clients);
void      clients_flush(const clients_t *const clients);
int       clients_send(const clients_t *const clients, const char *data,
		       const int length, const client_t *const except);
client_t *clients_get_client_from_name(const clients_t *const clients,
				       const char *const name);


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#endif /* !CLIENTS_H */

/* End of file */
