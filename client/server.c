/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: client/server.c
 *
 * Description: Server Managing Functions
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
#include <stdlib.h> /* atoi(), NULL       */
#include <stdio.h>  /* snprintf()         */
#include <unistd.h> /* close()            */
#include <string.h> /* memcpy(), strlen() */
#include <assert.h> /* assert()           */

/* Network-related headers */
#include <sys/types.h>
#include <sys/socket.h> /* connect()                */
#include <sys/select.h> /* fd_set, FD_CLR()         */
#include <netinet/in.h> /* htons()                  */
#include <arpa/inet.h>  /* inet_ntoa                */
#include <netdb.h>      /* hostent, gethostbyname() */

/* Project headers */
#include <common.h>
#include <iobuffer.h>
#include <hash.h>
#include "cltcmd.h"
#include "files.h"
#include "server.h"


/*****************************************************************************
 *
 * Constants
 *
 */

#ifndef DEFAULT_PORT
# define DEFAULT_PORT 4242
#endif


/*****************************************************************************
 *
 * Public functions
 *
 */

/*
 * Initialise the server handler.
 */
void server_init(server_t *const server, iobuffer_t *const console,
		 fd_set *const read_fds, fd_set *const write_fds,
		 int *const num_fds, struct files *const files)
{
    assert(server != NULL);
    assert(console != NULL);

    /* Initialize structure */
    server->sock = -1;
    server->console = console;
    server->files = files;
    server->read_fds = read_fds;
    server->write_fds = write_fds;
    server->num_fds = num_fds;
}

/*
 * Free the server handler.
 */
void server_free(server_t *const server)
{
    int sock;

    assert(server != NULL);

    iobuffer_free(&server->buffer);

    /* Close socket */
    if ((sock = server->sock) != -1) {
	close(sock);
	FD_CLR(sock, server->read_fds);
	FD_CLR(sock, server->write_fds);
	server->sock = -1;
    }
}

/*
 * Connect to a server.
 */
void server_connect(server_t *const server, const char *const nick,
		    const char *const address, const char *const port)
{
    int                len;            /* String length   */
    short              iport;          /* Port number     */
    char               str_buffer[40]; /* String buffer   */
    struct hostent    *host;           /* Server hostname */
    struct sockaddr_in addr;           /* Server address  */

    static const char msg_address[] = "Could not resolve server address.\n";
    static const char msg_socket[] = "Error: cannot create socket.\n";
    static const char msg_connect[] = "Connection failed.\n";
    static const char msg_connected[] = "Connected.\n";

    assert(server != NULL);
    assert(address != NULL);

    /* Resolve server name */
    if ((host = gethostbyname(address)) == NULL) {
	iobuffer_put_data(server->console, msg_address,
			  sizeof(msg_address) - 1);
	return;
    }

    /* Initialize address */
    iport = port == NULL ? DEFAULT_PORT : atoi(port);
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr, host->h_addr, sizeof(addr.sin_addr));
    addr.sin_port = htons(iport);

    /* Create socket */
    if ((server->sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
	iobuffer_put_data(server->console, msg_socket,
			  sizeof(msg_socket) - 1);
	return;
    }

    snprintf(str_buffer, sizeof(str_buffer), "Connecting to %s:%d...\n%n",
	     inet_ntoa(addr.sin_addr), iport, &len);
    iobuffer_put_data(server->console, str_buffer, len);

    /* Connect to server */
    if (connect(server->sock, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
	iobuffer_put_data(server->console, msg_connect,
			  sizeof(msg_connect) - 1);
	server->sock = -1;
	return;
    }

    /* Initialize I/O buffer */
    iobuffer_init(&server->buffer, server->sock, server->sock,
		  server->read_fds, server->write_fds, '\n');

    iobuffer_put_data(server->console, msg_connected,
		      sizeof(msg_connected) - 1);

    /* Give nickname to server */
    iobuffer_put_data(&server->buffer, "/connect ", 9);
    iobuffer_put_data(&server->buffer, nick, strlen(nick));
    iobuffer_put_data(&server->buffer, "\n", 1);

    if (*server->num_fds <= server->sock)
	*server->num_fds = server->sock + 1;
}

/*
 * Disconnect from the server.
 */
void server_disconnect(server_t *const server)
{
    static const char msg_quit[] = "/quit\n";

    /* Issue a `/quit' command */
    if (server->sock != -1) {
	iobuffer_put_data(&server->buffer, msg_quit, sizeof(msg_quit) - 1);
	server_free(server);
    }
}

/*
 * Read data from the server.
 */
void server_read(server_t *const server)
{
    int     cmd;  /* Command return value */
    line_t *line; /* Read line            */

    static const char msg_eof[] = "Disconnected from server.\n";

    if (server->sock == -1)
	return;

    /* Read data */
    if (iobuffer_read(&server->buffer) == 0) {
	/* Disconnect */
	iobuffer_put_data(server->console, msg_eof, sizeof(msg_eof) - 1);
	server_free(server);
    }

    cmd = 0;
    while (cmd == 0 &&
	   (line = iobuffer_input_line(&server->buffer, 0)) != NULL) {
	if (line->data[0] != '/')
	    /* Message */
	    iobuffer_put_data(server->console, line->data, line->length);
	else
	    /* Command */
	    cmd = cltcmd_exec(line->data + 1, CLTCMD_TYPE_SERVER,
			      server->console, server, server->files);

	free(line);
    }
}

/*
 * Write data to the server.
 */
void server_write(server_t *const server)
{
    if (server->sock == -1)
	return;

    iobuffer_write(&server->buffer);
}

/*
 * Put data in the server I/O buffer.
 */
int server_send(server_t *const server, const char *const data,
		const int length)
{
    if (server->sock != -1)
	return iobuffer_put_data(&server->buffer, data, length);
    return 0;
}

/* End of file */
