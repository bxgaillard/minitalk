/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: client/cltcmd.c
 *
 * Description: Client Command Processing
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
#include <stdlib.h> /* malloc(), free()                       */
#include <stdio.h>  /* snprintf()                             */
#include <string.h> /* strcmp(), strlen(), strchr(), memcpy() */
#include <assert.h> /* assert()                               */

/* Project headers */
#include <common.h>
#include <iobuffer.h>
#include <command.h>
#include "server.h"
#include "files.h"
#include "cltcmd.h"


/*****************************************************************************
 *
 * Data types
 *
 */

/* Data for callback functions */
typedef struct srvcmd_data {
    server_t *server; /* Pointer to the server handler */
    files_t  *files;  /* Pointer to the file handler   */
} cltcmd_data_t;


/*****************************************************************************
 *
 * Console commands
 *
 */

/*
 * Console `/connect', `/quit' and `/who' command (executed on server).
 */
static int cmd_cns_server(int arg_count, const char *const *args,
			  iobuffer_t *const console UNUSED,
			  iobuffer_t *const buffer UNUSED,
			  const cltcmd_data_t *const data)
{
    int         i;          /* Argument counter  */
    iobuffer_t *srv_buffer; /* Server I/O buffer */

    assert(arg_count > 0);
    assert(args != NULL);
    assert(args[0] != NULL);
    assert(data != NULL);

    srv_buffer = &data->server->buffer;

    iobuffer_put_data(srv_buffer, "/", 1);
    iobuffer_put_data(srv_buffer, args[0], strlen(args[0]));
    for (i = 1; i < arg_count; i++) {
	iobuffer_put_data(srv_buffer, " ", 1);
	iobuffer_put_data(srv_buffer, args[i], strlen(args[i]));
    }
    iobuffer_put_data(srv_buffer, "\n", 1);

    return 0;
}

/*
 * Console `/forbid' command.
 */
static int cmd_cns_forbid(int arg_count UNUSED, const char *const *args,
			  iobuffer_t *const console UNUSED,
			  iobuffer_t *const buffer UNUSED,
			  const cltcmd_data_t *const data)
{
    assert(arg_count == 2);
    assert(args != NULL);
    assert(data != NULL);

    return files_forbid(data->files, args[1]) == 0 ? 0 : 2;
}

/*
 * Console `/allow' command.
 */
static int cmd_cns_allow(int arg_count UNUSED, const char *const *args,
			 iobuffer_t *const console UNUSED,
			 iobuffer_t *const buffer UNUSED,
			 const cltcmd_data_t *const data)
{
    assert(arg_count == 2);
    assert(args != NULL);
    assert(data != NULL);

    files_allow(data->files, args[1]);
    return 0;
}

/*
 * Console `/mode' command.
 */
static int cmd_cns_mode(int arg_count UNUSED, const char *const *args,
			iobuffer_t *const console UNUSED,
			iobuffer_t *const buffer UNUSED,
			const cltcmd_data_t *const data)
{
    files_mode_t mode; /* File transfer mode */

    static const char msg_mode[] = "Invalid mode.  Valid ones are `secure' "
	"and `fast'.\n";

    assert(arg_count == 2);
    assert(args != NULL);
    assert(data != NULL);

    /* Select mode from argument */
    if (strcmp(args[1], "secure") == 0)
	mode = FILES_MODE_SECURE;
    else if (strcmp(args[1], "fast") == 0)
	mode = FILES_MODE_FAST;
    else {
	iobuffer_put_data(console, msg_mode, sizeof(msg_mode) - 1);
	return 0;
    }

    files_set_mode(data->files, mode);
    return 0;
}

/*
 * Console `/transfer' command.
 */
static int cmd_cns_transfer(int arg_count UNUSED, char **const args,
			    iobuffer_t *const console UNUSED,
			    iobuffer_t *const buffer UNUSED,
			    const cltcmd_data_t *const data)
{
    char *sep; /* Separator character */

    static const char msg_one[] = "There must be only and at most one local "
	"file and one remote file.\n";
    static const char msg_nick[] = "No nickname specified.\n";
    static const char msg_local[] = "No local file specified.\n";
    static const char msg_remote[] = "No remote file specified.\n";

    assert(arg_count == 3);
    assert(args != NULL);
    assert(data != NULL);

    if ((sep = strchr(args[1], ':')) != NULL) {
	if (strchr(args[2], ':') == NULL) {
	    /* Receive file */
	    if (sep == args[1])
		iobuffer_put_data(console, msg_nick, sizeof(msg_nick) - 1);
	    else if (args[2][0] == '\0')
		iobuffer_put_data(console, msg_local, sizeof(msg_local) - 1);
	    else if (sep[1] == '\0')
		iobuffer_put_data(console, msg_remote,
				  sizeof(msg_remote) - 1);
	    else {
		*sep = '\0';
		return files_req_receive(data->files, args[1], sep + 1,
					 args[2]);
	    }
	} else
	    iobuffer_put_data(console, msg_one, sizeof(msg_one) - 1);
    } else {
	if ((sep = strchr(args[2], ':')) != NULL) {
	    /* Send file */
	    if (sep == args[2])
		iobuffer_put_data(console, msg_nick, sizeof(msg_nick) - 1);
	    else if (args[1][0] == '\0')
		iobuffer_put_data(console, msg_local, sizeof(msg_local) - 1);
	    else if (sep[1] == '\0')
		iobuffer_put_data(console, msg_remote,
				  sizeof(msg_remote) - 1);
	    else {
		*sep = '\0';
		return files_req_send(data->files, args[2], args[1], sep + 1);
	    }
	} else
	    iobuffer_put_data(console, msg_one, sizeof(msg_one) - 1);
    }

    return 0;
}

/*
 * Console `/help' command.
 */
static int cmd_cns_help(int arg_count UNUSED, const char *const *args UNUSED,
			iobuffer_t *const console,
			iobuffer_t *const buffer UNUSED,
			const cltcmd_data_t *const data UNUSED)
{
    static const char msg_help[] =
	"/connect <nickname>: choose nickname once connected to a server.\n"
	"/who: get the currently connected user list.\n"
	"/allow <nickname>: allow a user to transfer files.\n"
	"/forbid <nickname>: forbid a user to transfer files.\n"
	"/mode {secure|fast}: select file transfer mode.\n"
	"/transfer <[user:]from> <[user:]to>: transfer a file from/to another"
	" user.\n"
	"/quit: disconnect from the server or quit the program.\n"
	"/help: get the command list.\n";

    assert(arg_count == 1);
    assert(args != NULL);
    assert(console != NULL);

    iobuffer_put_data(console, msg_help, sizeof(msg_help) - 1);
    return 0;
}


/*****************************************************************************
 *
 * Server commands
 *
 */

/*
 * Server `/receive' command.
 */
static int cmd_srv_receive(int arg_count UNUSED, const char *const *args,
			   iobuffer_t *const console UNUSED,
			   iobuffer_t *const buffer UNUSED,
			   const cltcmd_data_t *const data UNUSED)
{
    assert(arg_count == 5);
    assert(args != NULL);

    files_exec_receive(data->files, args[1], args[2], args[3], args[4]);
    return 0;
}

/*
 * Server `/send' command.
 */
static int cmd_srv_send(int arg_count UNUSED, const char *const *args,
			iobuffer_t *const console UNUSED,
			iobuffer_t *const buffer UNUSED,
			const cltcmd_data_t *const data UNUSED)
{
    assert(arg_count == 5);
    assert(args != NULL);

    files_exec_send(data->files, args[1], args[2], args[3], args[4]);
    return 0;
}

/*
 * Server `/accept' command.
 */
static int cmd_srv_accept(int arg_count UNUSED, const char *const *args,
			  iobuffer_t *const console UNUSED,
			  iobuffer_t *const buffer UNUSED,
			  const cltcmd_data_t *const data UNUSED)
{
    assert(arg_count == 6);
    assert(args != NULL);

    files_accept(data->files, args[1], args[2], args[3], args[4], args[5]);
    return 0;
}

/*
 * Server `/refuse' command.
 */
static int cmd_srv_refuse(int arg_count UNUSED, const char *const *args,
			  iobuffer_t *const console UNUSED,
			  iobuffer_t *const buffer UNUSED,
			  const cltcmd_data_t *const data UNUSED)
{
    int i; /* Counter in reason table */

    static const struct {
	char *name;
	char *full;
	int   length;
    } reasons[] = {
	{"open",    "File cannot be opened on the other side.\n",  41},
	{"create",  "File cannot be created on the other side.\n", 42},
	{"name",    "Invalid character in filename.\n",            31},
	{"nick",    "No such nickname.\n",                         18},
	{"forbid",  "User is forbidden.\n",                        19},
	{"id",      "File ID error.\n",                            15},
	{"connect", "Cannot connect.\n",                           16},
	{"host",    "Host address error.\n",                       20},
	{"intern",  "Internal error on the other side.\n",         34}
    };

    assert(arg_count == 4);
    assert(args != NULL);

    /* Print full reason */
    for (i = 0; i < (int) (sizeof(reasons) / sizeof(*reasons)); i++)
	if (strcmp(reasons[i].name, args[3]) == 0) {
	    iobuffer_put_data(console, reasons[i].full, reasons[i].length);
	    break;
	}

    files_refuse(data->files, args[2]);
    return 0;
}


/*****************************************************************************
 *
 * Command tables
 *
 */

/* Commands executed from console */
static const command_t console_commands[] = {
    {"allow",    1, "<nickname>",    (command_func_t) cmd_cns_allow   },
    {"connect",  1, "<nickname>",    (command_func_t) cmd_cns_server  },
    {"forbid",   1, "<nickname>",    (command_func_t) cmd_cns_forbid  },
    {"help",     0, NULL,            (command_func_t) cmd_cns_help    },
    {"mode",     1, "{secure|fast}", (command_func_t) cmd_cns_mode    },
    {"quit",     0, NULL,            (command_func_t) cmd_cns_server  },
    {"transfer", 2, "<[user:]from> <[user:]to>",
                                     (command_func_t) cmd_cns_transfer},
    {"who",      0, NULL,            (command_func_t) cmd_cns_server  }
};

/* Commands executed from server */
static const command_t server_commands[] = {
    {"accept",  5, "<nickname> <id1> <id2> <address> <port>",
     (command_func_t) cmd_srv_accept},
    {"receive", 4, "<nickname> <id> <mode> <filename>",
     (command_func_t) cmd_srv_receive},
    {"refuse",  3, "<nickname> <id> <reason>",
     (command_func_t) cmd_srv_refuse},
    {"send",    4, "<nickname> <id> <mode> <filename>",
     (command_func_t) cmd_srv_send}
};


/*****************************************************************************
 *
 * Public functions
 *
 */

/*
 * Execute a command.
 */
int cltcmd_exec(char *const command, cltcmd_type_t type,
		struct iobuffer *const console, struct server *const server,
		struct files *const files)
{
    int              count;    /* Command count               */
    const command_t *commands; /* Command list                */
    cltcmd_data_t    data;     /* Data for callback functions */

    assert(command != NULL);
    assert(type == CLTCMD_TYPE_CONSOLE || type == CLTCMD_TYPE_SERVER);
    assert(console != NULL);
    assert(server != NULL);

    /* Initialize callback data */
    data.server = server;
    data.files = files;

    /* Select the right command list */
    switch (type) {
    case CLTCMD_TYPE_CONSOLE:
	commands = console_commands;
	count = sizeof(console_commands) / sizeof(command_t);
	break;

    case CLTCMD_TYPE_SERVER:
	commands = server_commands;
	count = sizeof(server_commands) / sizeof(command_t);
	break;

    default:
	commands = NULL;
	count = 0;
    }

    /* Execute command */
    return command_exec(command, commands, count, console, console, &data);
}

/* End of file */
