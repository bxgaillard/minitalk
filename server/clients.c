/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: server/clients.c
 *
 * Description: Client Manager
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
#include <stdlib.h> /* malloc(), free(), NULL */
#include <stdio.h>  /* sprintf(), snprintf()  */
#include <unistd.h> /* close()                */
#include <string.h> /* strcmp(), strlen()     */
#include <assert.h> /* assert()               */

/* Network-related headers */
#include <sys/socket.h> /* accept()           */
#include <sys/select.h> /* fd_set, FD_*()     */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntoa()        */

/* Project headers */
#include <common.h>
#include <iobuffer.h>
#include <hash.h>
#include <command.h>
#include "srvcmd.h"
#include "clients.h"


/*****************************************************************************
 *
 * Private functions
 *
 */

/* Prototypes */
static int verify_nick(const char *const nick);
static int client_input_lines(client_t *const client,
			      clients_t *const clients);
static int client_auth_command(client_t *const client,
			       clients_t *const clients, int argc,
			       char **const argv);

/*
 * Verify nickname correctness.
 */
static int verify_nick(const char *const nick)
{
    int  i;   /* Index in string   */
    char chr; /* Current character */

    /* Test for invalid characters */
    for (i = 0; (chr = nick[i]) != '\0'; i++)
	if (chr == ':')
	    return 0;
    return i;
}

/*
 * Input messages and commands from clients.
 */
static int client_input_lines(client_t *const client,
			      clients_t *const clients)
{
    line_t *line;      /* Input line        */
    int     arg_count; /* Argument count    */
    int     len;       /* Command length    */
    char  **args;      /* Command arguments */

    /* Authentication message */
    static const char msg_auth[] = "You are not authenticated yet.  Use "
	"/connect to authenticate yourself.\n";

    assert(clients != NULL);
    assert(client != NULL);
    assert(clients->console != NULL);

    len = client->nick_len;

    while ((line = iobuffer_input_line(&client->buffer, len + 2)) != NULL) {
	if (line->data[0] != '/') {
	    /* Message */
	    if (client->nick_len > 0) {
		memcpy(line->start, client->nick, len);
		line->start[len] = ':';
		line->start[len + 1] = ' ';

		clients_send(clients, line->start, line->length + len + 2,
			     client);
	    } else
		iobuffer_put_data(&client->buffer, msg_auth,
				  sizeof(msg_auth) - 1);
	} else {
	    /* Command */
	    if (client->nick_len > 0)
		srvcmd_exec(line->data + 1, SRVCMD_TYPE_CLIENT,
			    clients->console, &client->buffer, clients,
			    client);
	    else {
		arg_count = command_get_tokens(line->data + 1, &args);

		if (client_auth_command(client, clients, arg_count, args)
		    == 1)
		    iobuffer_put_data(&client->buffer, msg_auth,
				      sizeof(msg_auth) - 1);

		if (args != NULL)
		    free(args);
	    }
	}

	free(line);
    }

    return 0;
}

/*
 * Input the authentication command from a connected client.
 */
static int client_auth_command(client_t *const client,
			       clients_t *const clients, int arg_count,
			       char **const args)
{
    int   len;    /* Nickname length */
    char *buffer; /* String buffer   */

    /* Messages */
    static const char msg_syntax[] = "Command error.  Syntax: "
	"/connect <nickname>\n";
    static const char msg_nick[] = "Nickname is not valid.  Issue another "
	"/connect command with a valid one.\n";
    static const char msg_taken[] = "Nickname is already taken.  Choose "
	"another one.\n";
    static const char msg_mem[] = "No more memory available.  Try another "
	"nickname.\n";

    assert(clients != NULL);
    assert(client != NULL);
    assert(clients->console != NULL);
    assert(client->nick == NULL);
    assert(client->nick_len == 0);

    /* Verify command */
    if (arg_count < 1 || strcmp(args[0], "connect") != 0)
	return 1;

    /* Verify syntax */
    if (arg_count != 2) {
	iobuffer_put_data(&client->buffer, msg_syntax,
			  sizeof(msg_syntax) - 1);
	return 2;
    }

    /* Verify nickname */
    if ((len = verify_nick(args[1])) == 0) {
	iobuffer_put_data(&client->buffer, msg_nick, sizeof(msg_nick) - 1);
	return 3;
    }

    /* Verify if nickname is not already taken */
    if (hash_find(&clients->hash, args[1]) != NULL) {
	iobuffer_put_data(&client->buffer, msg_taken, sizeof(msg_taken) - 1);
	return 4;
    }

    /* Allocate memory for nickname */
    if ((client->nick = malloc(len + 1)) == NULL) {
	iobuffer_put_data(&client->buffer, msg_mem, sizeof(msg_mem) - 1);
	return 5;
    }
    memcpy(client->nick, args[1], len + 1);

    /* Allocate memory for buffer */
    if ((buffer = malloc(client->addr_len + len + 32)) == NULL) {
	free(client->nick);
	client->nick = NULL;
	iobuffer_put_data(&client->buffer, msg_mem, sizeof(msg_mem) - 1);
	return 6;
    }

    client->nick_len = len;

    /* Add this client in the hash table */
    hash_add(&clients->hash, client->nick, client, &client->hash_elm);

    /* Send a message to other clients */
    sprintf(buffer, "** %s connected.\n", client->nick);
    clients_send(clients, buffer, len + 15, client);

    /* Write client info to the console */
    sprintf(buffer, "Client `%s' authenticated as `%s'.\n",
	    client->addr, client->nick);
    iobuffer_put_data(clients->console, buffer, client->addr_len + len + 31);

    /* Say hello to the new connected client */
    sprintf(buffer, "** Hello, %s!\n", client->nick);
    iobuffer_put_data(&client->buffer, buffer, len + 12);

    /* Free buffer */
    free(buffer);

    return 0;
}


/*****************************************************************************
 *
 * Public functions
 *
 */

/*
 * Initialize the client manager.
 */
void clients_init(clients_t *const clients, fd_set *const read_fds,
		  fd_set *const write_fds, struct iobuffer *const console,
		  const int srv_sock)
{
    clients->number = 0;
    clients->first = NULL;
    clients->last = NULL;
    clients->read_fds = read_fds;
    clients->write_fds = write_fds;
    clients->console = console;
    clients->srv_sock = srv_sock;

    hash_init(&clients->hash);
}

/*
 * Free the client manager.
 */
void clients_free(clients_t *const clients)
{
    client_t *client; /* Current client  */
    client_t *prev;   /* Previous client */

    assert(clients != NULL);

    /* Disconnect each client */
    client = clients->first;
    while (client != NULL) {
	close(iobuffer_get_input_fd(&client->buffer));
	iobuffer_free(&client->buffer);
	if (client->nick != NULL)
	    free(client->nick);

	prev = client;
	client = client->next;
	free(prev);
    }
    clients->number = 0;

    hash_free(&clients->hash);
}

/*
 * Add a connecting client.
 */
int clients_add(clients_t *const clients)
{
    int                sock;       /* Socket descriptor */
    int                len;        /* String length     */
    client_t          *client;     /* Current client    */
    char              *ip;         /* IP address        */
    socklen_t          addr_len;   /* Address length    */
    struct sockaddr_in addr;       /* Client address    */
    char               buffer[43]; /* String buffer     */

    assert(clients != NULL);

    /* Allocate memory for the new client structure */
    if ((client = malloc(sizeof(client_t))) == NULL)
	return -1;

    /* Accept the connection */
    addr_len = sizeof(client->addr);
    sock = accept(clients->srv_sock, (struct sockaddr *) &addr, &addr_len);
    ip = inet_ntoa(addr.sin_addr);
    snprintf(client->addr, sizeof(client->addr), "%s:%d%n", ip,
	     ntohs(addr.sin_port), &client->addr_len);

    /* Display client information on the console */
    snprintf(buffer, sizeof(buffer), "Client `%s' connected.\n%n",
	     client->addr, &len);
    iobuffer_put_data(clients->console, buffer, len);

    /* Initialize the structure */
    client->next = NULL;
    client->prev = clients->last;
    iobuffer_init(&client->buffer, sock, sock, clients->read_fds,
		  clients->write_fds, '\n');
    client->nick = NULL;
    client->nick_len = 0;

    /* Add this new element to the linked list */
    if (clients->first != NULL)
	clients->last->next = client;
    else
	clients->first = client;
    clients->last = client;
    clients->number++;

    return sock;
}

/*
 * Remove a disconnected client.
 */
void clients_remove(clients_t *const clients, client_t *const client)
{
    int   sock;       /* Socket descriptor */
    int   len;        /* String length     */
    char *name;       /* Client name       */
    char *str_buffer; /* String buffer     */

    assert(client != NULL);
    assert(clients != NULL);
    assert(clients->number != 0);

    sock = iobuffer_get_input_fd(&client->buffer);

    /* Close socket */
    close(sock);
    FD_CLR(sock, iobuffer_get_read_fds(&client->buffer));
    iobuffer_free(&client->buffer);

    /* Broadcast a message to tell that the client disconnected */
    name = client->nick != NULL ? client->nick : client->addr;
    len = strlen(name) + 25;
    if ((str_buffer = malloc(len)) != NULL) {
	snprintf(str_buffer, len, "Client `%s' disconnected.\n", name);
	iobuffer_put_data(clients->console, str_buffer, len);

	if (client->nick_len > 0) {
	    snprintf(str_buffer, len, "** %s disconnected.\n", name);
	    clients_send(clients, str_buffer, len - 6, client);
	}

	free(str_buffer);
    }

    /* Remove the client from the hash table */
    if (client->nick != NULL) {
	hash_remove(&clients->hash, &client->hash_elm);
	free(client->nick);
    }

    /* Unlink the client from the linked list */
    if (client->prev != NULL)
	client->prev->next = client->next;
    else
	clients->first = client->next;

    if (client->next != NULL)
	client->next->prev = client->prev;
    else
	clients->last = client->prev;

    free(client);
    clients->number--;
}

/*
 * Disconnect a client before removing it.
 */
void clients_disconnect(client_t *const client)
{
    assert(client != NULL);

    client->nick_len = -1;

    FD_CLR(iobuffer_get_input_fd(&client->buffer),
	   iobuffer_get_read_fds(&client->buffer));
}

/*
 * Read data from clients.
 */
int clients_read(clients_t *const clients)
{
    int       error;  /* Error indicator  */
    int       len;    /* Read data length */
    client_t *client; /* Current client   */
    client_t *next;   /* Next client      */

    assert(clients != NULL);
    assert(clients->console != NULL);

    error = 0;

    /* Input data for each client */
    for (client = clients->first; client != NULL; client = next) {
	next = client->next;

	if (client->nick_len != -1) {
	    len = iobuffer_read(&client->buffer);

	    if (len > 0) {
		if (client_input_lines(client, clients) != 0)
		    error = 1;
	    } else if (len != -2) {
		clients_remove(clients, client);
		if (len == -1)
		    error = 1;
	    }
	}
    }

    return error;
}

/*
 * Write data to clients.
 */
int clients_write(clients_t *const clients)
{
    int       error;  /* Error indicator */
    client_t *client; /* Current client  */
    client_t *prev;   /* Previous client */

    assert(clients != NULL);

    error = 0;

    /* Output data for each client */
    client = clients->first;
    while (client != NULL) {
	if (iobuffer_write(&client->buffer) == -1)
	    error = 1;
	if (client->nick_len == -1 &&
	    iobuffer_get_output_size(&client->buffer) == 0) {
	    prev = client;
	    client = client->next;
	    clients_remove(clients, prev);
	} else
	    client = client->next;
    }

    return error;
}

/*
 * Flush client output buffers.
 */
void clients_flush(const clients_t *const clients)
{
    client_t *client; /* Current client */

    assert(clients != NULL);

    /* Flush all buffers */
    for (client = clients->first; client != NULL; client = client->next) {
	FD_SET(iobuffer_get_output_fd(&client->buffer), clients->write_fds);
	iobuffer_write(&client->buffer);
    }
}

/*
 * Send a message to one or all clients
 */
int clients_send(const clients_t *const clients, const char *data,
		 const int length, const client_t *const except)
{
    int       error;  /* Error indicator */
    client_t *client; /* Current client  */

    assert(clients != NULL);
    assert(data != NULL);

    error = 0;

    /* For each client */
    for (client = clients->first; client != NULL; client = client->next)
	/* Client must be authenticated */
	if (client != except && client->nick_len > 0)
	    /* Send message */
	    if (iobuffer_put_data(&client->buffer, data, length) != length)
		error = 1;

    return error;
}

/*
 * Find a client by its name.
 */
client_t *clients_get_client_from_name(const clients_t *const clients,
				       const char *const name)
{
    hash_element_t *element; /* Hash table element */

    /* Search name */
    if ((element = hash_find(&clients->hash, name)) != NULL)
	return (client_t *) element->object;
    return NULL;
}

/* End of file */
