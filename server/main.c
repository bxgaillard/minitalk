/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: server/main.c
 *
 * Description: Server Main Functions
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
#include <stdlib.h> /* malloc(), free(), atoi()              */
#include <stdio.h>  /* perror(), printf(), fprintf(), stderr */
#include <unistd.h> /* close(), read(), write()              */
#include <assert.h> /* assert()                              */

/* Network-related headers */
#include <sys/socket.h> /* socket(), bind(), listen()                  */
#include <sys/select.h> /* select(), fd_set, FD_*()                    */
#include <netinet/in.h> /* struct sockaddr_in, INADDR_ANY, IPPROTO_TCP */

/* Project headers */
#include <common.h>
#include <iobuffer.h>
#include "clients.h"
#include "srvcmd.h"


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
	"Welcome to Minitalk Server!\n"
	"\n"
	"From here, you can send messages to all clients by typing text.\n"
	"Lines which begin with `/' are considered as commands.\n"
	"To get a comprehensive list of them, type `/help'.\n"
	"\n"
	"Have fun with Minitalk!\n"
	"\n";

    write(STDOUT_FILENO, msg_welcome, sizeof(msg_welcome) - 1);
}

/*
 * Create the server socket.
 */
static int create_socket(const unsigned short port)
{
    int                sock;     /* Created socket */
    socklen_t          addr_len; /* Address length */
    struct sockaddr_in addr;     /* Server address */

    /* Specify server address to use */
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    /* Create socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
	perror("Error while creating socket");
	return -1;
    }

    /* Bind socket */
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
	perror("Error while binding socket");
	close(sock);
	return -1;
    }

    /* Listen to the socket */
    if (listen(sock, 5) != 0) {
	perror("Error while listening to the socket");
	close(sock);
	return -1;
    }

    /* Display port information */
    addr_len = sizeof(addr);
    if (getsockname(sock, (struct sockaddr *) &addr, &addr_len) != 0) {
	perror("Error while getting socket informations");
	close(sock);
	return -1;
    }
    printf("Server is listening on port %u.\n\n", ntohs(addr.sin_port));

    return sock;
}

/*
 * Input messages and commands from console.
 */
static int console_input(clients_t *const clients, iobuffer_t *const console)
{
    int     cmd;  /* Command execution return value */
    line_t *line; /* Input line                     */

    /* Messages */
    static const char msg_eof_console[] = "EOF from standard input; "
	"exiting.\n";
    static const char msg_eof_clients[] = "** EOF from server standard input;"
	" closing connections.\n";

    /* EOF frop console */
    if (iobuffer_read(console) == 0) {
	clients_send(clients, msg_eof_clients, sizeof(msg_eof_clients) - 1,
		     NULL);
	iobuffer_put_data(console, msg_eof_console,
			  sizeof(msg_eof_console) - 1);
	return 1;
    }


    /* Analyze each line */
    cmd = 0;
    while ((line = iobuffer_input_line(console, 3)) != NULL && cmd == 0) {
	if (line->data[0] == '/')
	    /* Command */
	    cmd = srvcmd_exec(line->data + 1, SRVCMD_TYPE_SERVER, console,
			      console, clients, NULL);
	else {
	    /* Message */
	    line->start[0] = '*';
	    line->start[1] = '*';
	    line->start[2] = ' ';
	    clients_send(clients, line->start, line->length + 3, NULL);
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
    int        srv_sock; /* Server socket descriptor       */
    int        sock;     /* A socket descriptor            */
    int        nfds;     /* Number of descriptors          */
    fd_set     rfds;     /* Read descriptors for select()  */
    fd_set     wfds;     /* Write descriptors for select() */
    clients_t  clients;  /* Clients structure              */
    iobuffer_t console;  /* Console input/output buffer    */

    /* Verify parameters */
    if (argc > 2) {
	fprintf(stderr, "Usage: %s [port] (default %d)\n",
		argv[0], DEFAULT_PORT);
	return 1;
    }

    /* Write welcome message */
    write_welcome();

    /* Open server socket */
    srv_sock = create_socket(argc == 2 ? atoi(argv[1]) : DEFAULT_PORT);
    if (srv_sock == -1)
	return 2;

    /* Initialize structures */
    clients_init(&clients, &rfds, &wfds, &console, srv_sock);
    iobuffer_init(&console, STDIN_FILENO, STDOUT_FILENO, &rfds, &wfds, '\n');

    /* Initialize read descriptor set */
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    FD_SET(srv_sock, &rfds);

    /* Initialize write descriptor set */
    FD_ZERO(&wfds);
    FD_SET(STDOUT_FILENO, &wfds);

    /* Initialize descriptor number */
    nfds = srv_sock + 1;

    /* Main loop */
    while (1) {
	/* Wait for a ready descriptor */
	select(nfds, &rfds, &wfds, NULL, NULL);

	/* Check standard input stream */
	if (console_input(&clients, &console) != 0)
	    break;

	/* Check main server socket */
	if (FD_ISSET(srv_sock, &rfds)) {
	    /* Accept connection and add client */
	    sock = clients_add(&clients);

	    /* Update descriptor number */
	    if (sock >= nfds)
		nfds = sock + 1;
	} else
	    FD_SET(srv_sock, &rfds);

	/* Check client sockets */
	clients_read(&clients);

	/* Check output streams */
	clients_write(&clients);
	iobuffer_write(&console);
    }

    /* Flush buffers */
    FD_SET(STDOUT_FILENO, &wfds);
    iobuffer_write(&console);
    clients_flush(&clients);

    /* Free memory and close sockets */
    clients_free(&clients);
    iobuffer_free(&console);
    close(srv_sock);

    /* Exit silently */
    return 0;
}

/* End of file */
