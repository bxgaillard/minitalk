/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: server/srvcmd.h
 *
 * Description: Server Command Processing (Header)
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


#ifndef SRVCMD_H
#define SRVCMD_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*
 * Data types
 */

/* Non-explicit data types */
struct iobuffer;
struct clients;
struct client;

/* Command type (from the console or from a client) */
typedef enum srvcmd_type {
    SRVCMD_TYPE_SERVER, /* From console */
    SRVCMD_TYPE_CLIENT  /* From client  */
} srvcmd_type_t;


/*
 * Prototypes
 */

/* Public functions */
int srvcmd_exec(char *const command, srvcmd_type_t type,
		struct iobuffer *const console,
		struct iobuffer *const buffer, struct clients *const clients,
		struct client *const client);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !SRVCMD_H */

/* End of file */
