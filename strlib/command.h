/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: strlib/commands.h
 *
 * Description: Command Processing (Header)
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


#ifndef COMMAND_H
#define COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*
 * Data types
 */

/* Input/output buffer (non-explicit) */
struct iobuffer;

/* Command callback function */
typedef int (*command_func_t)(int arg_count, char **args,
			      struct iobuffer *const console,
			      struct iobuffer *const buffer, void *data);

/* Command structure */
typedef struct command {
    const char    *name;      /* Command name                     */
    int            arg_count; /* Argument count                   */
    const char    *syntax;    /* String describing command syntax */
    command_func_t function;  /* Callback function                */
} command_t;


/*
 * Prototypes
 */

int command_get_tokens(char *const command, char ***const args);
int command_exec(char *const command, const command_t *const commands,
		 const int count, struct iobuffer *const console,
		 struct iobuffer *const buffer, void *const data);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !COMMAND_H */

/* End of file */
