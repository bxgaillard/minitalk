/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: server/srvcmd.c
 *
 * Description: Server Command Processing
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
#include <string.h> /* strcmp(), strchr(), memcpy() */
#include <assert.h> /* assert()                     */

/* Project headers */
#include <common.h>
#include <iobuffer.h>
#include <command.h>
#include "clients.h"
#include "srvcmd.h"


/*****************************************************************************
 *
 * Data types
 *
 */

/* Data sent to callback functions */
typedef struct srvcmd_data {
    clients_t *clients; /* Client manager */
    client_t  *client;  /* Currect client */
} srvcmd_data_t;


/*****************************************************************************
 *
 * Server commands
 *
 */

/*
 * Console `/who' command.
 */
static int cmd_srv_who(int arg_count UNUSED, const char *const *args UNUSED,
		       iobuffer_t *const console UNUSED,
		       iobuffer_t *const buffer,
		       const srvcmd_data_t *const data)
{
    int       size;       /* String buffer size       */
    int       max;        /* Maximum printable number */
    int       len;        /* Number length            */
    char     *str_buffer; /* String buffer            */
    client_t *clt;        /* Current client           */

    static const char msg_none[] = "No client connected.\n";

    assert(arg_count == 1);
    assert(args != NULL);
    assert(buffer != NULL);
    assert(data != NULL);
    assert(data->clients != NULL);

    /* Calculate buffer size */
    size = 0;
    for (clt = data->clients->first; clt != NULL; clt = clt->next)
	if (clt->nick_len > 0)
	    size += clt->nick_len + 1;

    /* No client connected */
    if (size == 0) {
	iobuffer_put_data(buffer, msg_none, sizeof(msg_none) - 1);
	return 0;
    }

    /* Calculate the number length */
    max = 10;
    len = 1;
    while (max <= data->clients->number) {
	max *= 10;
	len++;
    }
    len += 32;

    if ((str_buffer = malloc(size + len)) == NULL)
	return 2;

    /* Output client number */
    size = len;
    snprintf(str_buffer, len + 1, "There are %d client(s) connected:\n",
	     data->clients->number);

    /* Output client names */
    for (clt = data->clients->first; clt != NULL; clt = clt->next)
	if (clt->nick_len > 0) {
	    memcpy(str_buffer + size, clt->nick, clt->nick_len);
	    str_buffer[size + clt->nick_len] = '\n';
	    size += clt->nick_len + 1;
	}
    iobuffer_put_data(buffer, str_buffer, size);

    free(str_buffer);

    return 0;
}

/*
 * Console `/kill' command.
 */
static int cmd_srv_kill(int arg_count UNUSED, const char *const *args,
			iobuffer_t *const console,
			iobuffer_t *const buffer UNUSED,
			const srvcmd_data_t *const data)
{
    int       len;        /* String buffer size */
    client_t *clt;        /* Current client     */
    char     *str_buffer; /* String buffer      */

    static const char msg_nick[] = "No such nickname.\n";
    static const char msg_you[] = "** You have been killed.\n";

    assert(arg_count == 2);
    assert(args != NULL);
    assert(args[1] != NULL);
    assert(console != NULL);
    assert(data != NULL);
    assert(data->clients != NULL);

    if ((clt = clients_get_client_from_name(data->clients, args[1]))
	== NULL) {
	iobuffer_put_data(console, msg_nick, sizeof(msg_nick) - 1);
	return 0;
    }

    len = clt->nick_len + 22;
    if ((str_buffer = malloc(len)) == NULL)
	return 2;

    iobuffer_put_data(&clt->buffer, msg_you, sizeof(msg_you) - 1);

    snprintf(str_buffer, len, "** %s has been killed.\n", clt->nick);
    clients_send(data->clients, str_buffer, len, clt);
    iobuffer_put_data(console, str_buffer + 3, len - 3);

    free(str_buffer);
    clients_disconnect(clt);
    return 0;
}

/*
 * Console `/shutdown' command.
 */
static int cmd_srv_shutdown(int arg_count UNUSED,
			    const char *const *args UNUSED,
			    iobuffer_t *const console UNUSED,
			    iobuffer_t *const buffer UNUSED,
			    const srvcmd_data_t *const data)
{
    static const char msg_shutdown[] = "Server is shutting down; closing "
	"connections.\n";

    assert(arg_count == 1);
    assert(args != NULL);
    assert(data != NULL);
    assert(data->clients != NULL);

    clients_send(data->clients, msg_shutdown, sizeof(msg_shutdown) - 1, NULL);
    return 1;
}

/*
 * Console `/help' command.
 */
static int cmd_srv_help(int arg_count UNUSED, const char *const *args UNUSED,
			iobuffer_t *const console UNUSED,
			iobuffer_t *const buffer,
			const srvcmd_data_t *const data UNUSED)
{
    static const char msg_help[] =
	"/who: get the list of the currently connected clients.\n"
	"/kill <nickname>: disconnect a client from the server.\n"
	"/shutdown: stop the server.\n"
	"/help: get the command list.\n";

    assert(arg_count == 1);
    assert(args != NULL);
    assert(buffer != NULL);

    iobuffer_put_data(buffer, msg_help, sizeof(msg_help) - 1);
    return 0;
}


/*****************************************************************************
 *
 * Client commands
 *
 */

/*
 * Client `/connect' command.
 */
static int cmd_clt_connect(int arg_count UNUSED,
			   const char *const *args UNUSED,
			   iobuffer_t *const console UNUSED,
			   iobuffer_t *const buffer,
			   const srvcmd_data_t *const data UNUSED)
{
    static const char msg_connected[] = "You are already connected.\n";

    assert(arg_count == 1);
    assert(args != NULL);
    assert(buffer != NULL);

    iobuffer_put_data(buffer, msg_connected, sizeof(msg_connected) - 1);
    return 0;
}

/*
 * Client `/quit' command.
 */
static int cmd_clt_quit(int arg_count UNUSED, const char *const *args UNUSED,
			iobuffer_t *const console,
			iobuffer_t *const buffer UNUSED,
			const srvcmd_data_t *const data)
{
    int   len;        /* String buffer length */
    char *str_buffer; /* String buffer        */

    static const char msg_mem[] = "Error: no more memory!\n";
    static const char msg_bye[] = "** Goodbye!\n";

    assert(arg_count == 1);
    assert(args != NULL);
    assert(console != NULL);
    assert(buffer != NULL);
    assert(data != NULL);
    assert(data->clients != NULL);
    assert(data->client != NULL);

    len = data->client->nick_len + 22;
    if ((str_buffer = malloc(len)) == NULL) {
	iobuffer_put_data(console, msg_mem, sizeof(msg_mem) - 1);
	return 1;
    }

    iobuffer_put_data(&data->client->buffer, msg_bye, sizeof(msg_bye) - 1);

    snprintf(str_buffer, len, "** %s has left server.\n", data->client->nick);
    clients_send(data->clients, str_buffer, len, data->client);
    iobuffer_put_data(console, str_buffer + 3, len - 3);

    free(str_buffer);
    clients_disconnect(data->client);
    return 0;
}

/*
 * Client `/receive', `/send' and `/refuse' commands.
 */
static int cmd_clt_p2p(int arg_count UNUSED, const char *const *args,
		       iobuffer_t *const console UNUSED,
		       iobuffer_t *const buffer,
		       const srvcmd_data_t *const data)
{
    int       i;   /* Argument counter */
    client_t *clt; /* Current client   */

    static const char msg_nick[] = " nick\nNo such nickname.\n";

    assert(arg_count > 2);
    assert(args != NULL);
    assert(args[0] != NULL);
    assert(args[1] != NULL);
    assert(args[2] != NULL);
    assert(data != NULL);

    if ((clt = clients_get_client_from_name(data->clients, args[1]))
	== NULL) {
	/* Error command */
	iobuffer_put_data(buffer, "/refuse ", 6);
	iobuffer_put_data(buffer, args[2], strlen(args[2]));

	/* End of error command and message */
	iobuffer_put_data(buffer, msg_nick, sizeof(msg_nick) - 1);
	return 0;
    }

    /* Command */
    iobuffer_put_data(&clt->buffer, "/", 1);
    iobuffer_put_data(&clt->buffer, args[0], strlen(args[0]));

    /* Nickname */
    iobuffer_put_data(&clt->buffer, " ", 1);
    iobuffer_put_data(&clt->buffer, data->client->nick,
		      data->client->nick_len);

    /* Other parameters */
    for (i = 2; i < arg_count; i++) {
	iobuffer_put_data(&clt->buffer, " ", 1);
	iobuffer_put_data(&clt->buffer, args[i], strlen(args[i]));
    }
    iobuffer_put_data(&clt->buffer, "\n", 1);

    return 0;
}

/*
 * Client `/accept' command.
 */
static int cmd_clt_accept(int arg_count UNUSED, const char *const *args,
			  iobuffer_t *const console UNUSED,
			  iobuffer_t *const buffer,
			  const srvcmd_data_t *const data)
{
    char     *sep; /* Separator character */
    client_t *clt; /* Current client      */

    static const char msg_nick[] = " nick\nNo such nickname.\n";

    assert(arg_count == 5);
    assert(args != NULL);
    assert(args[0] != NULL);
    assert(args[1] != NULL);
    assert(args[2] != NULL);
    assert(data != NULL);

    if ((clt = clients_get_client_from_name(data->clients, args[1]))
	== NULL) {
	/* Error command */
	iobuffer_put_data(buffer, "/refuse ", 6);
	iobuffer_put_data(buffer, args[2], strlen(args[2]));

	/* End of error command and message */
	iobuffer_put_data(buffer, msg_nick, sizeof(msg_nick) - 1);
	return 0;
    }

    /* Command */
    iobuffer_put_data(&clt->buffer, "/accept ", 8);

    /* Nickname */
    iobuffer_put_data(&clt->buffer, data->client->nick,
		      data->client->nick_len);

    /* IDs */
    iobuffer_put_data(&clt->buffer, " ", 1);
    iobuffer_put_data(&clt->buffer, args[2], strlen(args[2]));
    iobuffer_put_data(&clt->buffer, " ", 1);
    iobuffer_put_data(&clt->buffer, args[3], strlen(args[3]));

    /* Address */
    sep = strchr(data->client->addr, ':');
    iobuffer_put_data(&clt->buffer, " ", 1);
    iobuffer_put_data(&clt->buffer, data->client->addr,
		      sep - data->client->addr);

    /* Port */
    iobuffer_put_data(&clt->buffer, " ", 1);
    iobuffer_put_data(&clt->buffer, args[4], strlen(args[4]));
    iobuffer_put_data(&clt->buffer, "\n", 1);

    return 0;
}

/*
 * Client `/help' command.
 */
static int cmd_clt_help(int arg_count UNUSED, const char *const *args UNUSED,
			iobuffer_t *const console UNUSED,
			iobuffer_t *const buffer,
			const srvcmd_data_t *const data UNUSED)
{
    static const char msg_help[] =
	"/connect <nickname>: choose a nickname.\n"
	"/who: get the connected client list.\n"
	"/quit: disconnect from the server.\n"
	"/help: get the command list.\n"
	"/receive <nickname> <id> <mode> <filename>: recieve a file from a "
	"user.\n"
	"/send <nickname> <id> <mode> <filename>: send a file to another "
	"user.\n"
	"/accept <nickname> <id1> <id2> <port>: accept a file transfer.\n"
	"/refuse <nickname> <id> <reason>: refuse a file transfer.\n";

    assert(arg_count == 1);
    assert(args != NULL);
    assert(buffer != NULL);

    iobuffer_put_data(buffer, msg_help, sizeof(msg_help) - 1);
    return 0;
}


/*****************************************************************************
 *
 * Command tables
 *
 */

/* Console commands */
static const command_t server_commands[] = {
    {"help",     0, NULL,         (command_func_t) cmd_srv_help    },
    {"kill",     1, "<nickname>", (command_func_t) cmd_srv_kill    },
    {"shutdown", 0, NULL,         (command_func_t) cmd_srv_shutdown},
    {"who",      0, NULL,         (command_func_t) cmd_srv_who     }
};

/* Client commands */
static const command_t client_commands[] = {
    {"accept",  4, "<nickname> <id1> <id2> <port>",
                                      (command_func_t) cmd_clt_accept },
    {"connect", 1, "<nickname>",      (command_func_t) cmd_clt_connect},
    {"help",    0, NULL,              (command_func_t) cmd_clt_help   },
    {"quit",    0, NULL,              (command_func_t) cmd_clt_quit   },
    {"receive", 4, "<nickname> <id> <mode> <filename>",
                                      (command_func_t) cmd_clt_p2p    },
    {"refuse",  3, "<nickname> <id> <reason>",
                                      (command_func_t) cmd_clt_p2p    },
    {"send",    4, "<nickname> <id> <mode> <filename>",
                                      (command_func_t) cmd_clt_p2p    },
    {"who",     0, NULL,              (command_func_t) cmd_srv_who    }
                                      /* Same as server version */
};


/*****************************************************************************
 *
 * Public functions
 *
 */

/*
 * Execute a command.
 */
int srvcmd_exec(char *const command, srvcmd_type_t type,
		struct iobuffer *const console,
		struct iobuffer *const buffer, struct clients *const clients,
		struct client *const client)
{
    int              count;    /* Command count in list       */
    const command_t *commands; /* Command list                */
    srvcmd_data_t    data;     /* Data for callback functions */

    assert(command != NULL);
    assert(type == SRVCMD_TYPE_SERVER || type == SRVCMD_TYPE_CLIENT);
    assert(console != NULL);
    assert(buffer != NULL);
    assert(clients != NULL);

    data.clients = clients;
    data.client = client;

    /* Select the right command list and count */
    switch (type) {
    case SRVCMD_TYPE_SERVER:
	commands = server_commands;
	count = sizeof(server_commands) / sizeof(command_t);
	break;

    case SRVCMD_TYPE_CLIENT:
	commands = client_commands;
	count = sizeof(client_commands) / sizeof(command_t);
	break;

    default:
	commands = NULL;
	count = 0;
    }

    /* Execute command */
    return command_exec(command, commands, count, console, buffer, &data);
}

/* End of file */
