/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: client/server.h
 *
 * Description: Server Managing Functions (Header)
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


#ifndef SERVER_H
#define SERVER_H


/*
 * Headers
 */

/* System headers */
#include <sys/select.h> /* fd_set */

/* Project headers */
#include <iobuffer.h>
#include <hash.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*
 * Data types
 */

/* Non-explicit types */
struct files;

/* Server managing structure */
typedef struct server {
    int           sock;      /* Socket descriptor      */
    iobuffer_t    buffer;    /* Server I/O buffer      */
    iobuffer_t   *console;   /* Console I/O buffer     */
    struct files *files;     /* Files being transfered */
    fd_set       *read_fds;  /* Read descriptor set    */
    fd_set       *write_fds; /* Write descriptor set   */
    int          *num_fds;   /* Number of descriptors  */
} server_t;


/*
 * Prototypes
 */

/* Constructors and destructors */
void server_init(server_t *const server, iobuffer_t *const console,
		 fd_set *const read_fds, fd_set *const write_fds,
		 int *const num_fds, struct files *const files);
void server_free(server_t *const server);

/* Methods */
void server_connect(server_t *const server, const char *const nick,
		    const char *const address, const char *const port);
void server_disconnect(server_t *const server);
void server_read(server_t *const server);
void server_write(server_t *const server);
int  server_send(server_t *const server, const char *const data,
		 const int length);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !SERVER_H */
