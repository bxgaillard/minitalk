/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: client/main.c
 *
 * Description: Clien Main Functions
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
#include <stdlib.h> /* malloc(), free(), atoi(), srand() */
#include <stdio.h>  /* perror(), fprintf(), stderr       */
#include <unistd.h> /* close(), read(), write()          */
#include <string.h> /* strcmp()                          */
#include <time.h>   /* time()                            */
#include <assert.h> /* assert()                          */

/* Network-related headers */
#include <sys/types.h>
#include <sys/socket.h> /* socket(), bind(), listen()                  */
#include <sys/select.h> /* select(), fd_set, FD_*                      */
#include <netinet/in.h> /* struct sockaddr_in, INADDR_ANY, IPPROTO_TCP */

/* Project headers */
#include <common.h>
#include <iobuffer.h>
#include <command.h>
#include "cltcmd.h"
#include "files.h"
#include "server.h"


/*****************************************************************************
 *
 * Private functions
 *
 */

/*
 * Print a welcome message.
 */
static void write_welcome(void)
{
    static const char msg_welcome[] =
	"Minitalk: a basic talk-like client/server\n"
	"Copyright (C) 2004 Benjamin Gaillard\n"
	"\n"
	"Welcome to Minitalk Client!\n"
	"\n"
	"Issue a `/connect' command to connect yourself to a server.\n"
	"Once connected, type messages or commands.\n"
	"Commands start with a `/'; type `/help' to get a list.\n"
	"\n"
	"Have fun with Minitalk!\n"
	"\n";

    write(STDOUT_FILENO, msg_welcome, sizeof(msg_welcome) - 1);
}

/*
 * Input messages and commands from the console.
 */
static int console_input(iobuffer_t *const console, server_t *const server,
			 files_t *const files)
{
    int     cmd;       /* Command return value    */
    int     arg_count; /* Command argument count  */
    char  **args;      /* Command argument tokens */
    line_t *line;      /* Input line              */

    static const char msg_eof_console[] = "EOF from standard input; "
	"exiting.\n";
    static const char msg_connect[] = "Your are not connected yet.  Issue a "
	"/connect command to connect yourself.\n";
    static const char msg_syntax[] = "Command error.  Syntax: "
	"/connect <nickname> <address> [port]\n";
    static const char msg_none[] = "Wrong argument count.  This command "
	"takes none.\n";

    /* Console EOF */
    if (iobuffer_read(console) == 0) {
	server_disconnect(server);
	iobuffer_put_data(console, msg_eof_console,
			  sizeof(msg_eof_console) - 1);
	return 1;
    }

    cmd = 0;

    /* Input lines from the console */
    while (cmd == 0 && (line = iobuffer_input_line(console, 0)) != NULL) {
	if (server->sock != -1) {
	    if (line->data[0] != '/')
		/* Message */
		server_send(server, line->data, line->length);
	    else
		/* Command */
		cmd = cltcmd_exec(line->data + 1, CLTCMD_TYPE_CONSOLE,
				  console, server, files);
	} else {
	    if (line->data[0] == '/') {
		arg_count = command_get_tokens(line->data + 1, &args);

		if (arg_count > 0) {
		    /* Authentication, connection of quit command */
		    if (strcmp(args[0], "connect") == 0) {
			if (arg_count == 3 || arg_count == 4) {
			    server_connect(server, args[1], args[2],
					   arg_count == 3 ? NULL : args[3]);
			} else
			    iobuffer_put_data(console, msg_syntax,
					      sizeof(msg_syntax) - 1);
		    } else if (strcmp(args[0], "quit") == 0) {
			if (arg_count == 1)
			    return 1;
			else
			    iobuffer_put_data(console, msg_none,
					      sizeof(msg_none) - 1);
		    } else
			iobuffer_put_data(console, msg_connect,
					  sizeof(msg_connect) - 1);
		} else
		    iobuffer_put_data(console, msg_connect,
				      sizeof(msg_connect) - 1);

		free(args);
	    } else
		iobuffer_put_data(console, msg_connect,
				  sizeof(msg_connect) - 1);
	}

	free(line);
    }

    return cmd;
}


/*****************************************************************************
 *
 * Public functions
 *
 */

/*
 * Main function.
 */
int main(int argc, char *argv[])
{
    int        nfds;     /* Number of descriptors          */
    fd_set     rfds;     /* Read descriptors for select()  */
    fd_set     wfds;     /* Write descriptors for select() */
    iobuffer_t console;  /* Console input/output buffer    */
    server_t   server;   /* Server connection informations */
    files_t    files;    /* Files being transfered         */

    /* Verify parameters */
    if (argc != 1) {
	fprintf(stderr, "Usage: %s\n", argv[0]);
	return 1;
    }

    /* Write welcome message */
    write_welcome();

    /* Initialize structures */
    iobuffer_init(&console, STDIN_FILENO, STDOUT_FILENO, &rfds, &wfds, '\n');
    files_init(&files, &console);
    server_init(&server, &console, &rfds, &wfds, &nfds, &files);
    files_set_server(&files, &server);

    /* Initialize read descriptor sets */
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    FD_ZERO(&wfds);
    FD_SET(STDOUT_FILENO, &wfds);

    /* Initialize descriptor number */
    nfds = STDOUT_FILENO + 1;

    /* Initialize random number generator */
    srand(time(NULL));

    /* Main loop */
    while (1) {
	/* Wait for a ready descriptor */
	select(nfds, &rfds, &wfds, NULL, NULL);

	/* Transfer files */
	if (files_transfer(&files) != 0)
	    break;

	/* Check standard input stream */
	if (console_input(&console, &server, &files) != 0)
	    break;

	/* Chech server input stream */
	server_read(&server);

	/* Check output streams */
	iobuffer_write(&console);
	server_write(&server);
    }

    /* Flush buffers */
    FD_SET(STDOUT_FILENO, &wfds);
    server_write(&server);
    iobuffer_write(&console);

    /* Free memory and close sockets */
    iobuffer_free(&console);

    /* Exit silently */
    return 0;
}

/* End of file */
