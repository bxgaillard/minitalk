/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: client/files.c
 *
 * Description: Files Handling
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
#include <stdlib.h>   /* malloc(), free(), srand(), rand(), NULL */
#include <stdio.h>    /* snprintf()                              */
#include <fcntl.h>    /* open(), creat()                         */
#include <unistd.h>   /* close()                                 */
#include <string.h>   /* strlen(), strchr(), memcpy()            */
#include <time.h>     /* time()                                  */
#include <assert.h>   /* assert()                                */
#include <sys/stat.h> /* stat()                                  */

/* Network-related headers */
#include <sys/types.h>
#include <sys/socket.h> /* socket(), connect(), listen(), accept() */
#include <sys/select.h> /* FD_*()                                  */
#include <netinet/in.h> /* sockaddr_in, IPPROTO_TCP, IPPROTO_UDP   */
#include <netdb.h>      /* hostent, gethostbyname()                */

/* Project headers */
#include <common.h>
#include <iobuffer.h>
#include <hash.h>
#include "server.h"
#include "files.h"


/*****************************************************************************
 *
 * Constants
 *
 */

/* Key length for file transfers */
#ifndef FILE_KEY_LENGTH
# define FILE_KEY_LENGTH 16
#endif


/*****************************************************************************
 *
 * Data types
 *
 */

/* Forbidden user nickname */
typedef struct nick {
    struct nick   *prev;    /* Previous element in linked list */
    struct nick   *next;    /* Next element in linked list     */
    char          *nick;    /* Pointer to the nickname         */
    hash_element_t element; /* Element in hash table           */
} nick_t;

/* File transfer direction */
typedef enum file_dir {
    FILE_DIR_RECEIVE, /* Receive direction */
    FILE_DIR_SEND     /* Send direction    */
} file_dir_t;

/* File transfer structure */
typedef struct file {
    struct file   *prev;     /* Previous element in linked list   */
    struct file   *next;     /* Next element in linked list       */
    files_mode_t   mode;     /* Tranfer mode (secure/fast)        */
    file_dir_t     dir;      /* Transfer direction (receive/send) */

    int            from_fd;  /* Read file descriptor              */
    int            to_fd;    /* Write file descriptor             */
    int            sock_fd;  /* Socket descriptor                 */

    int            nick_len; /* Peer nickname length              */
    int            name_len; /* Peer filename length              */

    char          *key;      /* File ID                           */
    char          *nick;     /* Peer nikname                      */
    char          *name;     /* Peer flename                      */

    hash_element_t element;
} file_t;


/*****************************************************************************
 *
 * Private functions
 *
 */

/* Prototypes */
static void    generate_key(char *const buffer, const int length);
static int     verify_filename(const char *const name);
static file_t *file_new(files_t *const files, const char *const nick,
			const char *const name, const files_mode_t mode,
			const file_dir_t dir);
static void    file_delete(files_t *const files, file_t *const file);
static file_t *file_find(const files_t *const files, const char *const key);
static int     create_socket(const files_mode_t mode, unsigned short *port);
static void    send_transfer_init(files_t *const files, file_t *const file);
static void    send_accept(files_t *const files, file_t *const file,
			   const char *const key, const unsigned short port);
static void    send_refuse(files_t *const files, const char *const nick,
			   const char *const key, const char *const reason);

/*
 * Generate a random file ID.
 */
static void generate_key(char *const buffer, const int length)
{
    int i; /* Index in generated string */

    static const char pool[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz+-";

    assert(buffer != NULL);
    assert(length >= 0);

    /* Write each character */
    for (i = 0; i < length; i++)
	buffer[i] = pool[rand() % (sizeof(pool) - 1)];
    buffer[i] = '\0';
}

/*
 * Verify filename correctness.
 */
static int verify_filename(const char *const name)
{
    int i;    /* Index in string   */
    char chr; /* Current character */

    /* Empty filename or hidden file */
    chr = name[0];
    if (chr == '\0' || chr == '.')
	return 1;

    /* No path in filename */
    for (i = 1; (chr = name[i]) != '\0'; i++)
	if (chr == '/')
	    return 2;

    return 0;
}

/*
 * Create a new file transfer (initiated locally or remotely).
 */
static file_t *file_new(files_t *const files, const char *const nick,
			const char *const name, const files_mode_t mode,
			const file_dir_t dir)
{
    int     nick_len; /* Peer nickname length    */
    int     name_len; /* Peer filename length    */
    file_t *file;     /* File transfer structure */

    assert(files != NULL);
    assert(nick != NULL);
    assert(name != NULL);
    assert(dir == FILE_DIR_RECEIVE || dir == FILE_DIR_SEND);
    assert(files->mode == FILES_MODE_SECURE ||
	   files->mode == FILES_MODE_FAST);

    nick_len = strlen(nick) + 1;
    name_len = strlen(name) + 1;

    /* Allocate memory */
    if ((file = malloc(nick_len + name_len +
		       (sizeof(file_t) + FILE_KEY_LENGTH + 1))) == NULL)
	return NULL;
    file->key = (char *) (file + 1);

    /* Generate random key */
    generate_key(file->key, FILE_KEY_LENGTH);
    while (hash_find(&files->file_keys, file->key) != NULL) {
	srand(time(NULL));
	generate_key(file->key, FILE_KEY_LENGTH);
    }
    hash_add(&files->file_keys, file->key, file, &file->element);

    /* Initialize structure */
    file->prev = NULL;
    file->next = files->files;
    files->files = file;

    file->dir = dir;
    file->mode = mode;

    file->nick_len = nick_len - 1;
    file->name_len = name_len - 1;
    file->nick = file->key + (FILE_KEY_LENGTH + 1);
    file->name = file->nick + nick_len;

    memcpy(file->nick, nick, nick_len);
    memcpy(file->name, name, name_len);

    return file;
}

/*
 * Delete and terminate a file transfer.
 */
static void file_delete(files_t *const files, file_t *const file)
{
    assert(files != NULL);
    assert(file != NULL);

    /* Remove file ID in hash table */
    hash_remove(&files->file_keys, &file->element);

    /* Free file/socket descriptors */
    if (file->from_fd != -1) {
	FD_CLR(file->from_fd, files->server->read_fds);
	close(file->from_fd);
    }
    if (file->to_fd != -1) {
	FD_CLR(file->to_fd, files->server->write_fds);
	close(file->to_fd);
    }
    if (file->sock_fd != -1) {
	FD_CLR(file->sock_fd, files->server->read_fds);
	close(file->sock_fd);
    }

    /* Unlink from linked list */
    if (file->prev != NULL)
	file->prev->next = file->next;
    else
	files->files = file->next;
    if (file->next != NULL)
	file->next->prev = file->prev;

    free(file);
}

/*
 * Find a file transfer by its key.
 */
static file_t *file_find(const files_t *const files, const char *const key)
{
    hash_element_t *element;

    assert(files != NULL);
    assert(key != NULL);

    /* Find through hash table */
    if ((element = hash_find(&files->file_keys, key)) != NULL)
	return (file_t *) element->object;
    return NULL;
}

/*
 * Create a server socket (file transfer initiated remotely).
 */
static int create_socket(const files_mode_t mode, unsigned short *port)
{
    int                sock; /* Socket descriptor                  */
    socklen_t          len;  /* Address length                     */
    struct sockaddr_in addr; /* Server (for file transfer) address */

    assert(mode == FILES_MODE_SECURE || mode == FILES_MODE_FAST);
    assert(port != NULL);
    
    /* Select transfer mode */
    switch (mode) {
    case FILES_MODE_SECURE:
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	break;

    case FILES_MODE_FAST:
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	break;

    default:
	return -1;
    }

    if (sock == -1)
	return -1;

    /* Listen on all interfaces */
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(0);

    /* Bind and listen to the socket */
    len = sizeof(addr);
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) != 0 ||
	(mode == FILES_MODE_SECURE && listen(sock, 1) != 0) ||
	getsockname(sock, (struct sockaddr *) &addr, &len) != 0) {
	close(sock);
	return -1;
    }

    *port = ntohs(addr.sin_port);
    return sock;
}

/*
 * Send a transfer initialization command through the server.
 */
static void send_transfer_init(files_t *const files, file_t *const file)
{
    assert(files != NULL);
    assert(file != NULL);

    /* Select the appropriate command */
    switch (file->dir) {
    case FILE_DIR_RECEIVE:
	server_send(files->server, "/receive ", 9);
	break;

    case FILE_DIR_SEND:
	server_send(files->server, "/send ", 6);
    }

    /* Command */
    server_send(files->server, file->nick, file->nick_len);
    server_send(files->server, " ", 1);
    server_send(files->server, file->key, FILE_KEY_LENGTH);

    /* Mode */
    switch (files->mode) {
    case FILES_MODE_SECURE:
	server_send(files->server, " secure ", 8);
	break;

    case FILES_MODE_FAST:
	server_send(files->server, " fast ", 6);
    }

    /* Filename */
    server_send(files->server, file->name, file->name_len);
    server_send(files->server, "\n", 1);
}

/*
 * Send an `/accept' command through the server.
 */
static void send_accept(files_t *const files, file_t *const file,
			const char *const key, const unsigned short port)
{
    int  len;       /* String buffer length */
    char buffer[8]; /* String buffer        */

    assert(files != NULL);
    assert(file != NULL);
    assert(key != NULL);

    /* Command */
    server_send(files->server, "/accept ", 8);

    /* Nickname */
    server_send(files->server, file->nick, file->nick_len);

    /* Keys */
    server_send(files->server, " ", 1);
    server_send(files->server, key, strlen(key));
    server_send(files->server, " ", 1);
    server_send(files->server, file->key, FILE_KEY_LENGTH);

    /* Port */
    snprintf(buffer, sizeof(buffer), " %u\n%n", port, &len);
    server_send(files->server, buffer, len);
}

/*
 * Send a `/refuse' command through the server.
 */
static void send_refuse(files_t *const files, const char *const nick,
			const char *const key, const char *const reason)
{
    assert(files != NULL);
    assert(nick != NULL);
    assert(key != NULL);
    assert(reason != NULL);

    /* Command */
    server_send(files->server, "/refuse ", 8);

    /* Nickname */
    server_send(files->server, nick, strlen(nick));

    /* Key */
    server_send(files->server, " ", 1);
    server_send(files->server, key, strlen(key));

    /* Reason */
    server_send(files->server, " ", 1);
    server_send(files->server, reason, strlen(reason));
    server_send(files->server, "\n", 1);
}


/*****************************************************************************
 *
 * Public functions
 *
 */

/*
 * Initialize the file transfer handler.
 */
void files_init(files_t *const files, struct iobuffer *const console)
{
    assert(files != NULL);
    assert(console != NULL);

    /* Initialize structure */
    files->nicks = NULL;
    files->console = console;

    /* Initialize hash tables */
    hash_init(&files->forbid);
    hash_init(&files->file_keys);
}

/*
 * Free the file transfer handler.
 */
void files_free(files_t *const files)
{
    hash_free(&files->forbid);
    hash_free(&files->file_keys);
}

/*
 * Set the server we are connected to.
 */
void files_set_server(files_t *const files, struct server *const server)
{
    assert(files != NULL);
    assert(server != NULL);

    files->server = server;
}

/*
 * Forbid a user to transfer files to/from us.
 */
int files_forbid(files_t *const files, const char *const nick)
{
    int     len;   /* Nickname length    */
    nick_t *snick; /* Nickname structure */

    static const char msg_already[] = "User already forbidden.\n";

    assert(files != NULL);
    assert(nick != NULL);

    if (hash_find(&files->forbid, nick) == NULL) {
	/* Allocate memory */
	len = strlen(nick) + 1;
	if ((snick = malloc(len + sizeof(nick_t))) == NULL)
	    return 1;

	/* Add to linked list */
	snick->prev = NULL;
	snick->next = files->nicks;
	snick->nick = (char *) (snick + 1);
	memcpy(snick->nick, nick, len);

	/* Add to hash table */
	hash_add(&files->forbid, snick->nick, snick, &snick->element);
    } else
	iobuffer_put_data(files->console, msg_already,
			  sizeof(msg_already) - 1);

    return 0;
}

/*
 * Allow a user to transfer files to/from us.
 */
void files_allow(files_t *const files, const char *const nick)
{
    hash_element_t *element; /* Hash table element */
    nick_t         *snick;   /* Nickname structure */

    static const char msg_not[] = "User not forbidden.\n";

    assert(files != NULL);
    assert(nick != NULL);

    if ((element = hash_find(&files->forbid, nick)) == NULL) {
	iobuffer_put_data(files->console, msg_not, sizeof(msg_not) - 1);
	return;
    }

    snick = (nick_t *) element->object;

    /* Remove from linked list */
    if (snick->prev != NULL)
	snick->prev->next = snick->next;
    else
	files->nicks = snick->next;
    if (snick->next != NULL)
	snick->next->prev = snick->prev;

    /* Remove from hash table */
    hash_remove(&files->forbid, element);
    free(snick);
}

/*
 * Remove all forbidden users.
 */
void files_reset_forbidden(files_t *const files)
{
    nick_t *nick; /* Current element */
    nick_t *next; /* Next element    */

    assert(files != NULL);

    /* Empty linked list */
    next = NULL;
    for (nick = files->nicks; nick != NULL; nick = next) {
	nick = nick->next;
	free(nick);
    }

    files->nicks = NULL;
    hash_init(&files->forbid);
}

/*
 * Let know if a user is forbidden.
 */
int files_is_forbidden(const files_t *const files, const char *const nick)
{
    assert(files != NULL);
    assert(nick != NULL);

    /* Search through hash table */
    return hash_find(&files->forbid, nick) != NULL;
}

/*
 * Set transfer mode (secure or fast).
 */
void files_set_mode(files_t *const files, const files_mode_t mode)
{
    assert(files != NULL);
    assert(mode == FILES_MODE_SECURE || mode == FILES_MODE_FAST);

    files->mode = mode;
}

/*
 * Send a request to receive a file from a user with a `/receive' command.
 */
int files_req_receive(files_t *const files, const char *const nick,
		      const char *const from, const char *const to)
{
    int         fd;    /* File descriptor         */
    file_t     *file;  /* File transfer structure */
    struct stat sstat; /* File statistics         */

    static const char msg_invalid[] = "Error: invalid filename.\n";
    static const char msg_exists[] = "Error: file already exists.\n";
    static const char msg_create[] = "Error: cannot create file.\n";

    assert(files != NULL);
    assert(files->server != NULL);
    assert(nick != NULL);
    assert(from != NULL);
    assert(to != NULL);

    if (verify_filename(from) != 0 || verify_filename(to) != 0) {
	iobuffer_put_data(files->console, msg_invalid,
			  sizeof(msg_invalid) - 1);
	return 0;
    }

    if (stat(to, &sstat) == 0) {
	iobuffer_put_data(files->console, msg_exists, sizeof(msg_exists) - 1);
	return 0;
    }

    if ((fd = creat(to, 0666)) == -1) {
	iobuffer_put_data(files->console, msg_create, sizeof(msg_create) - 1);
	return 0;
    }

    if ((file = file_new(files, nick, from, files->mode, FILE_DIR_RECEIVE))
	== NULL) {
	close(fd);
	return 2;
    }
    file->from_fd = -1;
    file->to_fd = fd;
    file->sock_fd = -1;
    send_transfer_init(files, file);

    return 0;
}

/*
 * Send a request to send a file to a user with a `/send' command.
 */
int files_req_send(files_t *const files, const char *const nick,
		   const char *const from, const char *const to)
{
    int     fd;   /* File descriptor         */
    file_t *file; /* File transfer structure */

    static const char msg_invalid[] = "Error: invalid filename.\n";
    static const char msg_open[] = "Error: cannot open file.\n";

    assert(files != NULL);
    assert(files->server != NULL);
    assert(nick != NULL);
    assert(from != NULL);
    assert(to != NULL);

    if (verify_filename(from) != 0 || verify_filename(to) != 0) {
	iobuffer_put_data(files->console, msg_invalid,
			  sizeof(msg_invalid) - 1);
	return 0;
    }

    if ((fd = open(from, O_RDONLY)) == -1) {
	iobuffer_put_data(files->console, msg_open, sizeof(msg_open) - 1);
	return 0;
    }

    if ((file = file_new(files, nick, to, files->mode, FILE_DIR_SEND))
	== NULL) {
	close(fd);
	return 2;
    }
    file->from_fd = fd;
    file->to_fd = -1;
    file->sock_fd = -1;
    send_transfer_init(files, file);

    return 0;
}

/*
 * Execute a `/receive' command sent by another user.
 */
int files_exec_receive(files_t *const files, const char *const nick,
		       const char *const key, const char *const mode,
		       const char *const name)
{
    int            fd;     /* File descriptor         */
    int            sock;   /* Socket descriptor       */
    int            len;    /* String buffer length    */
    unsigned short port;   /* Port number             */
    files_mode_t   fmode;  /* File transfer mode      */
    file_t        *file;   /* File transfer structure */
    char          *buffer; /* String buffer           */

    assert(files != NULL);
    assert(nick != NULL);
    assert(key != NULL);
    assert(mode != NULL);
    assert(name != NULL);

    if (strcmp(mode, "secure") == 0)
	fmode = FILES_MODE_SECURE;
    else if (strcmp(mode, "fast") == 0)
	fmode = FILES_MODE_FAST;
    else {
	send_refuse(files, nick, key, "mode");
	return 0;
    }

    if (verify_filename(name) != 0) {
	send_refuse(files, nick, key, "name");
	return 0;
    }

    len = strlen(nick) + strlen(name) + 32;
    if ((buffer = malloc(len)) == NULL) {
	send_refuse(files, nick, key, "intern");
	return 1;
    }

    if (files_is_forbidden(files, nick)) {
	snprintf(buffer, len, "%s attempted to get the `%s' file.\n%n",
		 nick, name, &len);
	iobuffer_put_data(files->console, buffer, len);
	send_refuse(files, nick, key, "forbid");
	free(buffer);
	return 0;
    }

    if ((fd = open(name, O_RDONLY)) == -1) {
	snprintf(buffer, len, "%s attempted to get the `%s' file.\n%n",
		 nick, name, &len);
	iobuffer_put_data(files->console, buffer, len);
	send_refuse(files, nick, key, "open");
	free(buffer);
	return 0;
    }

    if ((file = file_new(files, nick, name, fmode, FILE_DIR_SEND)) == NULL ||
	(sock = create_socket(fmode, &port)) == -1) {
	send_refuse(files, nick, key, "intern");
	close(fd);
	free(buffer);
	return 2;
    }

    snprintf(buffer, len, "%s is getting the `%s' file.\n%n", nick, name,
	     &len);
    iobuffer_put_data(files->console, buffer, len);

    file->from_fd = fd;
    file->to_fd = -1;
    file->sock_fd = sock;
    FD_SET(sock, files->server->read_fds);

    if (sock >= *files->server->num_fds)
	*files->server->num_fds = sock + 1;

    send_accept(files, file, key, port);
    free(buffer);
    return 0;
}

/*
 * Execute a `/sent' command sent by another user.
 */
int files_exec_send(files_t *const files, const char *const nick,
		    const char *const key, const char *const mode,
		    const char *const name)
{
    int            fd;     /* File descriptor         */
    int            sock;   /* Socket descriptor       */
    int            len;    /* String buffer length    */
    unsigned short port;   /* Port number             */
    files_mode_t   fmode;  /* File transfer mode      */
    file_t        *file;   /* File transfer structure */
    char          *buffer; /* String buffer           */
    struct stat    sstat;  /* File statistics         */

    assert(files != NULL);
    assert(nick != NULL);
    assert(key != NULL);
    assert(mode != NULL);
    assert(name != NULL);

    if (strcmp(mode, "secure") == 0)
	fmode = FILES_MODE_SECURE;
    else if (strcmp(mode, "fast") == 0)
	fmode = FILES_MODE_FAST;
    else {
	send_refuse(files, nick, key, "mode");
	return 0;
    }

    len = strlen(nick) + strlen(name) + 33;
    if ((buffer = malloc(len)) == NULL) {
	send_refuse(files, nick, key, "intern");
	return 1;
    }

    if (files_is_forbidden(files, nick)) {
	snprintf(buffer, len, "%s attempted to send the `%s' file.\n%n",
		 nick, name, &len);
	iobuffer_put_data(files->console, buffer, len);
	send_refuse(files, nick, key, "forbid");
	free(buffer);
	return 0;
    }

    if (stat(name, &sstat) == 0) {
	snprintf(buffer, len, "%s attempted to send the `%s' file.\n%n",
		 nick, name, &len);
	send_refuse(files, nick, key, "exists");
	free(buffer);
	return 0;
    }

    if ((fd = creat(name, 0666)) == -1) {
	send_refuse(files, nick, key, "create");
	free(buffer);
	return 0;
    }

    if ((file = file_new(files, nick, name, fmode, FILE_DIR_RECEIVE)) == NULL
	|| (sock = create_socket(fmode, &port)) == -1) {
	send_refuse(files, nick, key, "intern");
	close(fd);
	free(buffer);
	return 2;
    }

    snprintf(buffer, len, "%s is sending the `%s' file.\n%n", nick, name,
	     &len);
    iobuffer_put_data(files->console, buffer, len);

    file->to_fd = fd;
    if (fmode == FILES_MODE_SECURE) {
	file->from_fd = -1;
	file->sock_fd = sock;
    } else {
	file->from_fd = sock;
	file->sock_fd = -1;
    }
    FD_SET(sock, files->server->read_fds);

    if (sock >= *files->server->num_fds)
	*files->server->num_fds = sock + 1;

    send_accept(files, file, key, port);
    free(buffer);
    return 0;
}

/*
 * Initiate a file transfer after having received an `/accept' command.
 */
int files_accept(files_t *const files, const char *const nick,
		 const char *const key, const char *const host_key,
		 const char *const address, const char *const port)
{
    int                sock;  /* Socket descriptor       */
    unsigned short     iport; /* Port number             */
    file_t            *file;  /* File transfer structure */
    struct hostent    *host;  /* Host name resolver      */
    struct sockaddr_in addr;  /* Peer address            */

    static const char msg_connect[] = "Error while connecting to host.\n";
    static const char msg_accept[] = "File transfer accepted.  Transfer "
	"initiated.\n";

    if ((file = file_find(files, key)) == NULL) {
	send_refuse(files, nick, host_key, "id");
	return 0;
    }

    /* Resolve peer address */
    if ((host = gethostbyname(address)) == NULL) {
	send_refuse(files, nick, host_key, "host");
	iobuffer_put_data(files->console, msg_connect,
			  sizeof(msg_connect) - 1);

	file_delete(files, file);
	return 0;
    }

    iport = atoi(port);

    if ((sock = file->mode == FILES_MODE_SECURE ?
	 socket(PF_INET, SOCK_STREAM, IPPROTO_TCP) :
	 socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
	iobuffer_put_data(files->console, msg_connect,
			  sizeof(msg_connect) - 1);
	send_refuse(files, nick, host_key, "intern");
	return 1;
    }

    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr, host->h_addr, sizeof(addr.sin_addr));
    addr.sin_port = htons(iport);

    /* Connect to peer */
    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
	iobuffer_put_data(files->console, msg_connect,
			  sizeof(msg_connect) - 1);
	send_refuse(files, nick, host_key, "connect");
	return 1;
    }

    /* Set file descriptor in file transfer structure */
    switch (file->dir) {
    case FILE_DIR_RECEIVE:
	file->from_fd = sock;

	if (file->mode == FILES_MODE_FAST) {
	    file->sock_fd = sock;
	    FD_SET(sock, files->server->write_fds);
	} else
	    FD_SET(sock, files->server->read_fds);
	break;

    case FILE_DIR_SEND:
	file->to_fd = sock;
	FD_SET(sock, files->server->write_fds);
    }

    /* Update file descriptor number */
    if (sock >= *files->server->num_fds)
	*files->server->num_fds = sock + 1;

    iobuffer_put_data(files->console, msg_accept, sizeof(msg_accept) - 1);
    return 0;
}

/*
 * Abord a file transfer after having received a `/refuse' command.
 */
int files_refuse(files_t *const files, const char *const key)
{
    file_t *file; /* File transfer structure */

    static const char msg_refuse[] = "File transfer refused.\n";

    /* Find the corresponding structure and delete it */
    if ((file = file_find(files, key)) != NULL)
	file_delete(files, file);

    iobuffer_put_data(files->console, msg_refuse, sizeof(msg_refuse) - 1);
    return 0;
}

/*
 * Read/write data for each file transfer.
 */
int files_transfer(files_t *const files)
{
    int                sock;         /* Socket descriptor       */
    int                len;          /* Number of read bytes    */
    int                written;      /* Number of written bytes */
    file_t            *file;         /* Current file transfer   */
    file_t            *next;         /* Next file transfer      */
    char               buffer[1024]; /* File transfer buffer    */
    socklen_t          addr_len;     /* Peer address length     */
    struct sockaddr_in addr;         /* Peer address (UDP)      */

    static const char msg_data[] = "Arbitrary data to initiate transfer.";
    static const char msg_success[] = "File succesfully transfered.\n";

    assert(files != NULL);

    for (file = files->files; file != NULL; file = next) {
	next = file->next;

	/* UDP file receive initalization */
	if (file->mode == FILES_MODE_FAST && file->dir == FILE_DIR_RECEIVE &&
	    file->sock_fd != -1) {
	    write(file->sock_fd, msg_data, sizeof(msg_data) - 1);

	    FD_SET(file->sock_fd, files->server->read_fds);
	    FD_CLR(file->sock_fd, files->server->write_fds);

	    file->sock_fd = -1;
	    continue;
	}

	if ((file->from_fd != -1 &&
	     FD_ISSET(file->from_fd, files->server->read_fds)) ||
	    (file->to_fd != -1 &&
	     FD_ISSET(file->to_fd, files->server->write_fds))) {
	    /* Socket is ready to be read or written */
	    switch (file->mode) {
	    case FILES_MODE_SECURE:
		/* Secure mode */
		if ((len = read(file->from_fd, buffer, sizeof(buffer)))
		    == 0) {
		    /* Stream end */
		    iobuffer_put_data(files->console, msg_success,
				      sizeof(msg_success) - 1);
		    file_delete(files, file);
		    continue;
		}
		if (len == -1)
		    return 1;

		/* Read and write until read buffer is empty */
		do {
		    written = write(file->to_fd, buffer, len);
		    if (written == -1)
			return 1;
		    if (written != len && file->dir == FILE_DIR_SEND)
			lseek(file->from_fd, written - len, SEEK_CUR);

		    len = read(file->from_fd, buffer, sizeof(buffer));
		} while (len == sizeof(buffer));

		if (len == -1)
		    return 1;
		if (len > 0)
		    write(file->to_fd, buffer, len);
		break;

	    case FILES_MODE_FAST:
		/* Fast mode */
		switch (file->dir) {
		case FILE_DIR_RECEIVE:
		    /* Read one datagram */
		    len = read(file->from_fd, buffer, sizeof(buffer));
		    if (len == -1)
			return 1;
		    if (len != 1 && write(file->to_fd, buffer + 1,
					  len - 1) == -1)
			return 1;

		    if (len != sizeof(buffer) || buffer[0] != '\0') {
			iobuffer_put_data(files->console, msg_success,
					  sizeof(msg_success) - 1);
			file_delete(files, file);
		    }
		    break;

		case FILE_DIR_SEND:
		    /* Close sock if file is complete */
		    if (file->sock_fd == -2) {
			file->sock_fd = -1;
			iobuffer_put_data(files->console, msg_success,
					  sizeof(msg_success) - 1);
			file_delete(files, file);
			break;
		    }

		    /* Write one datagram */
		    len = read(file->from_fd, buffer + 1, sizeof(buffer) - 1);
		    if (len == -1)
			return 1;

		    if (len == sizeof(buffer) - 1) {
			/* Beginning or middle of file */
			buffer[0] = '\0';
			write(file->to_fd, buffer, sizeof(buffer));
		    } else {
			/* End of file */
			buffer[0] = '\1';
			write(file->to_fd, buffer, len + 1);
			file->sock_fd = -2;
		    }
		}
	    }
	} else {
	    /* No read or write can be done on this socket */
	    if (file->sock_fd != -1 && file->sock_fd != -2 &&
		(file->from_fd == -1 || file->to_fd == -1)) {
		if (FD_ISSET(file->sock_fd, files->server->read_fds)) {
		    /* Peer is connecting to our listening socket */
		    if (file->mode == FILES_MODE_SECURE) {
			/* Secure mode: client connected */
			addr_len = sizeof(addr);
			sock = accept(file->sock_fd,
				      (struct sockaddr *) &addr, &addr_len);
			if (sock == -1)
			    return 1;
			if (sock >= *files->server->num_fds)
			    *files->server->num_fds = sock + 1;
			FD_CLR(file->sock_fd, files->server->read_fds);

			switch (file->dir) {
			case FILE_DIR_RECEIVE:
			    file->from_fd = sock;
			    FD_SET(sock, files->server->read_fds);
			    break;

			case FILE_DIR_SEND:
			    file->to_fd = sock;
			    FD_SET(sock, files->server->write_fds);
			}
		    } else {
			/* Fast mode: we received an initiating datagram */
			addr_len = sizeof(addr);
			if (recvfrom(file->sock_fd, buffer, sizeof(buffer), 0,
				     (struct sockaddr *) &addr, &addr_len)
			    == -1)
			    return -1;
			if (connect(file->sock_fd, (struct sockaddr *) &addr,
				    addr_len) != 0)
			    return 1;

			FD_CLR(file->sock_fd, files->server->read_fds);
			FD_SET(file->sock_fd, files->server->write_fds);
			file->to_fd = file->sock_fd;
			file->sock_fd = -1;
		    }
		} else
		    FD_SET(file->sock_fd, files->server->read_fds);

		continue;
	    }

	    /* No read/write/connection: reset bits in descriptor sets */
	    switch (file->dir) {
	    case FILE_DIR_RECEIVE:
		if (file->from_fd != -1)
		    FD_SET(file->from_fd, files->server->read_fds);
		if (file->mode == FILES_MODE_FAST &&
		    file->sock_fd != -1 && file->sock_fd != -2)
		    FD_SET(file->from_fd, files->server->write_fds);
		break;

	    case FILE_DIR_SEND:
		if (file->to_fd != -1)
		    FD_SET(file->to_fd, files->server->write_fds);
	    }
	}
    }

    return 0;
}

/* End of file */
