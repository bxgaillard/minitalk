/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: strlib/commands.c
 *
 * Description: Command Processing
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
#include <stdlib.h> /* malloc(), free()             */
#include <stdio.h>  /* snprintf()                   */
#include <unistd.h> /* write()                      */
#include <string.h> /* strcmp(), strlen(), memcpy() */
#include <assert.h> /* assert()                     */

/* Project headers */
#include <common.h>
#include <iobuffer.h>
#include "command.h"


/*****************************************************************************
 *
 * Local functions
 *
 */

/* Prototypes */
static const command_t *command_find(const char *const command,
				     const command_t *const commands,
				     const int count);

/*
 * Search a command in a provided command list.
 */
static const command_t *command_find(const char *const command,
				     const command_t *const commands,
				     const int count)
{
    int start;
    int end;
    int pos;
    int cmp;

    assert(command != NULL);
    assert(commands != NULL);

    start = 0;
    end = count - 1;
    pos = end / 2;

    /* Dichotomic search */
    while ((cmp = strcmp(command, commands[pos].name)) != 0 && start < end) {
	if (cmp < 0)
	    end = pos - 1;
	else
	    start = pos + 1;
	pos = (start + end) / 2;
    }

    if (cmp == 0)
	return commands + pos;
    return NULL;
}


/*****************************************************************************
 *
 * Public functions
 *
 */

/*
 * Split a command line in different tokens (blank-character-separated).
 */
int command_get_tokens(char *const command, char ***const args)
{
    int i;
    int count;
    char chr;

    assert(command != NULL);
    assert(args != NULL);

    count = 0;
    i = 0;
    chr = command[0];

    /* Count arguments */
    while (chr != '\n') {
	while ((chr == ' ' || chr == '\t') && chr != '\n')
	    chr = command[++i];

	if (chr != '\n')
	    count++;
	else
	    break;

	do
	    chr = command[++i];
	while (chr != ' ' && chr != '\t' && chr != '\n');
    }

    if (count == 0)
	return 0;

    /* Allocate argument table */
    if ((*args = malloc(count * sizeof(**args))) == NULL)
	return -1;

    count = 0;
    i = 0;
    chr = command[0];

    /* Split arguments and fill argument table */
    while (chr != '\n') {
	while ((chr == ' ' || chr == '\t') && chr != '\n')
	    chr = command[++i];

	if (chr != '\n')
	    (*args)[count++] = command + i;
	else
	    break;

	do
	    chr = command[++i];
	while (chr != ' ' && chr != '\t' && chr != '\n');
	command[i] = '\0';
    }

    return count;
}

/**
 * Execute a command line in the provided command list, with given parameters.
 */
int command_exec(char *const command, const command_t *const commands,
		 const int count, struct iobuffer *const console,
		 struct iobuffer *const buffer, void *const data)
{
    int arg_count;
    const command_t *cmd;
    int res;
    char **args;

    /* Error messages */
    static const char msg_no_cmd[] = "No command entered.  Syntax: "
	"/command [arg 1] [arg 2] ... [arg n]\nType `/help' to get a command "
	"list.\n";
    static const char msg_mem[] = "Error: no more memory!\n";
    static const char msg_count[] = "Wrong argument count";
    static const char msg_none[] = ": this command takes none.\n";
    static const char msg_unknown[] = "Unknown command.  Type `/help' to get "
	"a command list.\n";

    assert(command != NULL);
    assert(commands != NULL);

    /* Split arguments */
    arg_count = command_get_tokens(command, &args);

    if (arg_count == 0) {
	if (buffer != NULL)
	    iobuffer_put_data(buffer, msg_no_cmd, sizeof(msg_no_cmd) - 1);
	return 0;
    }
    if (arg_count == -1) {
	if (buffer != NULL)
	    iobuffer_put_data(buffer, msg_mem, sizeof(msg_mem) - 1);
	return 2;
    }

    if ((cmd = command_find(args[0], commands, count)) != NULL) {
	if (arg_count == cmd->arg_count + 1) {
	    /* Correct syntax: execute command */
	    res = cmd->function(arg_count, args, console, buffer, data);

	    if (res == 2) {
		/* Memory error */
		if (buffer != NULL)
		    iobuffer_put_data(buffer, msg_mem, sizeof(msg_mem) - 1);
		if (console != buffer)
		    iobuffer_put_data(console, msg_mem, sizeof(msg_mem) - 1);
	    }
	} else {
	    /* Print the syntax error */
	    if (buffer != NULL) {
		iobuffer_put_data(buffer, msg_count, sizeof(msg_count) - 1);
		if (cmd->arg_count != 0) {
		    iobuffer_put_data(buffer, ".  Syntax: /", 12);
		    iobuffer_put_data(buffer, args[0], strlen(args[0]));
		    iobuffer_put_data(buffer, " ", 1);
		    iobuffer_put_data(buffer, cmd->syntax,
				      strlen(cmd->syntax));
		    iobuffer_put_data(buffer, "\n", 1);
		} else
		    iobuffer_put_data(buffer, msg_none, sizeof(msg_none) - 1);
	    }

	    res = 0;
	}
    } else {
	if (buffer != NULL)
	    iobuffer_put_data(buffer, msg_unknown, sizeof(msg_unknown) - 1);
	res = 0;
    }

    /* Free memory (because I'm worth it) */
    free(args);

    return res;
}

/* End of file */
