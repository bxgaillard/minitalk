/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: client/files.h
 *
 * Description: Files Handling (Headers)
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


#ifndef FILES_H
#define FILES_H


/*
 * Headers
 */

/* Project headers */
#include <hash.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*
 * Data types
 */

/* Non-explicit types */
struct iobuffer;
struct server;
struct nick;

/* File transfer mode */
typedef enum files_mode {
    FILES_MODE_SECURE, /* Secure mode (TCP) */
    FILES_MODE_FAST    /* Fast mode (UDP)   */
} files_mode_t;

/* Files handler structure */
typedef struct files {
    struct nick     *nicks;     /* Forbidden users list       */
    struct file     *files;     /* Files being transfered     */
    struct iobuffer *console;   /* Console I/O buffer         */
    struct server   *server;    /* Pointer to server handler  */
    files_mode_t     mode;      /* File transfer mode         */
    hash_t           forbid;    /* Forbidden users hash table */
    hash_t           file_keys; /* File keys hash table       */
} files_t;


/*
 * Prototypes
 */

/* Constructors and destructors */
void files_init(files_t *const files, struct iobuffer *const console);
void files_free(files_t *const files);
void files_set_server(files_t *const files, struct server *const server);

/* Authorization methods */
int  files_forbid(files_t *const files, const char *const nick);
void files_allow(files_t *const files, const char *const nick);
void files_reset_forbidden(files_t *const files);
int  files_is_forbidden(const files_t *const files, const char *const nick);

/* Methods called from commands */
void files_set_mode(files_t *const files, const files_mode_t mode);
int  files_req_receive(files_t *const files, const char *const nick,
		       const char *const from, const char *const to);
int  files_req_send(files_t *const files, const char *const nick,
		    const char *const from, const char *const to);
int  files_exec_receive(files_t *const files, const char *const nick,
			const char *const key, const char *const mode,
			const char *const name);
int  files_exec_send(files_t *const files, const char *const nick,
		     const char *const key, const char *const mode,
		     const char *const name);
int  files_accept(files_t *const files, const char *const nick,
		  const char *const key, const char *const host_key,
		  const char *const address, const char *const port);
int  files_refuse(files_t *const files, const char *const nickname);

/* Method controlling files transfering */
int files_transfer(files_t *const files);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !FILES_H */
