/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: dbuffer.c
 *
 * Description: Dynamic Buffers
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
#include <unistd.h> /* read(), write()        */
#include <string.h> /* memcpy()               */
#include <assert.h> /* assert()               */

/* Network-related headers */
#include <sys/select.h> /* fd_set, FD_*() */

/* Project headers */
#include <common.h>
#include "dbuffer.h"


/*****************************************************************************
 *
 * Constants
 *
 */

#ifndef BUFFER_SIZE
# define BUFFER_SIZE 256
#endif


/*****************************************************************************
 *
 * Data types
 *
 */

/* Internal buffer */
typedef struct ibuffer {
    struct ibuffer *next;               /* Next element in linked list      */
    int             start;              /* Start of buffer in data (offset) */
    int             end;                /* End of buffer in data (padding)  */
    char            data[BUFFER_SIZE];  /* Data containing the buffer       */
} ibuffer_t;


/*****************************************************************************
 *
 * Local functions
 *
 */

/* Prototypes */
static ibuffer_t *ibuffer_new(void);

/*
 * Create a new internal buffer.
 */
static ibuffer_t *ibuffer_new(void)
{
    ibuffer_t *ibuffer;

    /* Allocate buffer */
    if ((ibuffer = malloc(sizeof(ibuffer_t))) == NULL)
	return NULL;

    /* Initialize buffer */
    ibuffer->next = NULL;
    ibuffer->start = 0;
    ibuffer->end = 0;

    return ibuffer;
}


/*****************************************************************************
 *
 * Global functions
 *
 */

/*
 * Create a new buffer.
 */
dbuffer_t *dbuffer_new(const int fd, fd_set *const fds, const char separator)
{
    dbuffer_t *buffer;

    if ((buffer = malloc(sizeof(dbuffer_t))) != NULL)
	dbuffer_init(buffer, fd, fds, separator);
    return buffer;
}

/*
 * Delete a buffer.
 */
void dbuffer_delete(dbuffer_t *const buffer)
{
    assert(buffer != NULL);

    dbuffer_free(buffer);
    free(buffer);
}

/*
 * Initialize a buffer.
 */
void dbuffer_init(dbuffer_t *const buffer, const int fd, fd_set *const fds,
		  const char separator)
{
    assert(buffer != NULL);

    buffer->first = NULL;
    buffer->last = NULL;
    buffer->size = 0;
    buffer->fd = fd;
    buffer->fds = fds;
    buffer->separator = separator;
}

/*
 * Free a buffer.
 */
void dbuffer_free(dbuffer_t *const buffer)
{
    ibuffer_t *ibuffer;
    ibuffer_t *prev;

    assert(buffer != NULL);
    ibuffer = buffer->first;

    /* Free internal buffers */
    while (ibuffer != NULL) {
	prev = ibuffer;
	ibuffer = ibuffer->next;
	free(prev);
    }

    buffer->first = NULL;
    buffer->last = NULL;
    buffer->size = 0;
}

/*
 * Get the buffer size.
 */
int dbuffer_get_size(const dbuffer_t *const buffer)
{
    assert(buffer != NULL);

    return buffer->size;
}

/*
 * Get the file descriptor.
 */
int dbuffer_get_fd(const dbuffer_t *const buffer)
{
    assert(buffer != NULL);

    return buffer->fd;
}

/*
 * Get the file descriptor set.
 */
fd_set *dbuffer_get_fds(const dbuffer_t *const buffer)
{
    assert(buffer != NULL);

    return buffer->fds;
}

/*
 * Get the separator character.
 */
char dbuffer_get_separator(const dbuffer_t *const buffer)
{
    assert(buffer != NULL);

    return buffer->separator;
}

/*
 * Set the file descriptor.
 */
void dbuffer_set_fd(dbuffer_t *const buffer, const int fd)
{
    assert(buffer != NULL);

    buffer->fd = fd;
}

/*
 * Set the file descriptor set.
 */
void dbuffer_set_fds(dbuffer_t *const buffer, fd_set *const fds)
{
    assert(buffer != NULL);

    buffer->fds = fds;
}

/*
 * Set the separator character.
 */
void dbuffer_set_separator(dbuffer_t *const buffer, const char separator)
{
    assert(buffer != NULL);

    buffer->separator = separator;
}

/*
 * Read data and put it into the buffer.
 */
int dbuffer_read(dbuffer_t *const buffer)
{
    int        total;   /* Number of read bytes                   */
    int        empty;   /* Size of empty buffer space             */
    int        len;     /* Read data length                       */
    ibuffer_t *ibuffer; /* Internal buffer                        */
    ibuffer_t *prev;    /* Previous internal buffer (linked list) */

    assert(buffer != NULL);

    if (buffer->fds != NULL && !FD_ISSET(buffer->fd, buffer->fds)) {
	FD_SET(buffer->fd, buffer->fds);
	return -2;
    }

    /* Allocate first internal buffer if necessary */
    if (buffer->first == NULL) {
	if ((ibuffer = ibuffer_new()) == NULL)
	    return -1;
	buffer->first = ibuffer;
	buffer->last = ibuffer;
    } else
	ibuffer = buffer->last;

    total = 0;
    prev = NULL;

    while (1) {
	/* Read data */
	empty = sizeof(ibuffer->data) - ibuffer->end;
	len = read(buffer->fd, ibuffer->data + ibuffer->end, empty);

	if (len == 0) {
	    /* End of data */
	    if (ibuffer->end == 0 && prev != NULL) {
		prev->next = NULL;
		buffer->last = prev;
		if (buffer->first == ibuffer)
		    buffer->first = NULL;
		free(ibuffer);
	    }
	    break;
	}
	if (len == -1)
	    break;

	empty -= len;
	total += len;
	ibuffer->end += len;

	if (empty != 0)
	    break;

	/* Allocate a new internal buffer */
	prev = ibuffer;
	if ((ibuffer->next = ibuffer_new()) == NULL)
	    break;
	buffer->last = ibuffer->next;
	ibuffer = ibuffer->next;
    }

    buffer->size += total;
    return total;
}

/*
 * Write data from the buffer.
 */
int dbuffer_write(dbuffer_t *const buffer)
{
    int   len;  /* Written data length     */
    int   size; /* Number of written bytes */
    char *data; /* Pointer to buffer       */

    assert(buffer != NULL);

    if (buffer->fds != NULL && !FD_ISSET(buffer->fd, buffer->fds)) {
	if (buffer->size != 0)
	    FD_SET(buffer->fd, buffer->fds);
	return -2;
    }

    if ((size = buffer->size) == 0 || buffer->first == NULL) {
	if (buffer->fds != NULL)
	    FD_CLR(buffer->fd, buffer->fds);
	return 0;
    }
    if ((data = malloc(size)) == NULL)
	return -1;

    /* TODO: improve this (do not use dbuffer_*) */

    /* Write data and re-add the remaining data (not written yet) */
    dbuffer_get_data(buffer, data, size);
    len = write(buffer->fd, data, size);
    if (len == size) {
	if (buffer->fds != NULL)
	    FD_CLR(buffer->fd, buffer->fds);
    } else
	dbuffer_put_data(buffer, data + len, size - len);
    free(data);

    return len;
}

/*
 * Get the size of the first token in the buffer.
 */
int dbuffer_token_size(const dbuffer_t *const buffer)
{
    int        i;         /* Counter             */
    int        size;      /* Size of token       */
    char       chr;       /* Character           */
    char       separator; /* Separator character */
    ibuffer_t *ibuffer;   /* Internal buffer     */

    assert(buffer != NULL);
    separator = buffer->separator;

    size = 0;
    chr = '\0';
    ibuffer = buffer->first;

    /* Search for the separator character in the buffer */
    while (ibuffer != NULL) {
	for (i = ibuffer->start; i < ibuffer->end; i++) {
	    size++;
	    if ((chr = ibuffer->data[i]) == separator)
		break;
	}

	if (chr == separator)
	    break;
	ibuffer = ibuffer->next;
    }

    if (ibuffer != NULL)
	return size;
    return 0;
}

/*
 * Get data from the buffer.
 */
int dbuffer_get_data(dbuffer_t *const buffer, char *const data,
		     const int data_size)
{
    int        size;        /* Size of remaining data  */
    int        done;        /* Number of get bytes     */
    int        buffer_size; /* Size of internal buffer */
    int        copy_size;   /* Number of bytes to copy */
    ibuffer_t *ibuffer;     /* Internal buffer         */

    assert(buffer != NULL);
    size = data_size;

    if (size == 0 || (ibuffer = buffer->first) == NULL)
	return 0;

    /* Copy data from the first internal buffer */
    buffer_size = ibuffer->end - ibuffer->start;
    copy_size = size <= buffer_size ? size : buffer_size;

    if (data != NULL)
	memcpy(data, ibuffer->data + ibuffer->start, copy_size);
    done = copy_size;
    size -= copy_size;

    /* Copy data from the other internal buffers */
    while (1) {
	if (copy_size != buffer_size) {
	    ibuffer->start += copy_size;
	    break;
	}

	buffer->first = ibuffer->next;
	free(ibuffer);
	ibuffer = buffer->first;

	if (ibuffer == NULL) {
	    buffer->last = NULL;
	    break;
	}
	if (size == 0)
	    break;

	buffer_size = ibuffer->end;
	copy_size = size <= buffer_size ? size : buffer_size;

	if (data != NULL)
	    memcpy(data + done, ibuffer->data, copy_size);
	done += copy_size;
	size -= copy_size;
    }

    buffer->size -= done;
    return done;
}

/*
 * Append data to the buffer.
 */
int dbuffer_put_data(dbuffer_t *const buffer, const char *const data,
		     const int data_size)
{
    int        total;   /* Total transferred bytes        */
    int        empty;   /* Empty space in internal buffer */
    int        len;     /* Size of remaining data         */
    ibuffer_t *ibuffer; /* Internal buffer                */

    assert(buffer != NULL);

    /* Allocate first buffer if necessary */
    if (buffer->first == NULL) {
	if ((ibuffer = ibuffer_new()) == NULL)
	    return -1;
	buffer->first = ibuffer;
	buffer->last = ibuffer;
    } else
	ibuffer = buffer->last;

    total = 0;
    len = data_size;

    while (1) {
	empty = sizeof(ibuffer->data) - ibuffer->end;

	/* Copy all remaining data */
	if (len < empty) {
	    memcpy(ibuffer->data + ibuffer->end, data + total, len);
	    ibuffer->end += len;
	    total += len;
	    break;
	}

	/* Copy data */
	memcpy(ibuffer->data + ibuffer->end, data + total, empty);
	ibuffer->end += empty;
	total += empty;
	len -= empty;

	/* Allocate a new buffer */
	if (len != 0) {
	    if ((ibuffer->next = ibuffer_new()) == NULL)
		break;
	    buffer->last = ibuffer->next;
	    ibuffer = ibuffer->next;
	} else
	    break;
    }

    buffer->size += total;
    return total;
}

/*
 * Input a non-blank line from the buffer.
 */
line_t *dbuffer_input_line(dbuffer_t *const buffer, const int space)
{
    int     len;  /* Line length */
    line_t *line; /* Line data   */

    /* While there is a line in the buffer */
    while ((len = dbuffer_token_size(buffer)) != 0)
	if (len != 1) {
	    /* Get line */
	    if ((line = malloc(sizeof(line_t) + space + len)) == NULL)
		return NULL;
	    line->length = len;
	    line->start = (char *) (line + 1);
	    line->data = line->start + space;
	    dbuffer_get_data(buffer, line->data, len);

	    /* Return if we found a non-blank line */
	    if (len != 2 || line->data[0] != '\r') {
		if (line->data[len - 2] == '\r')
		    line->data[len-- - 2] = '\n';
		line->length = len;
		return line;
	    }
	    free(line);
	} else
	    /* Drop it if it is a blank one */
	    dbuffer_get_data(buffer, NULL, 1);

    return NULL;
}

/* End of file */
