/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: iobuffer.c
 *
 * Description: Dynamic Input/Output Buffers
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
#include <stdlib.h>     /* malloc(), free(), NULL */
#include <assert.h>     /* assert()               */
#include <sys/select.h> /* fd_set                 */

/* Project headers */
#include <common.h>
#include "dbuffer.h"
#include "iobuffer.h"


/*****************************************************************************
 *
 * Public functions
 *
 */

/*
 * Create a new I/O Buffer.
 */
iobuffer_t *iobuffer_new(const int input_fd, const int output_fd,
			 fd_set *const read_fds, fd_set *const write_fds,
			 const char separator)
{
    iobuffer_t *buffer;

    if ((buffer = malloc(sizeof(iobuffer_t))) != NULL)
	iobuffer_init(buffer, input_fd, output_fd, read_fds, write_fds,
		      separator);
    return buffer;
}

/*
 * Delete an I/O Buffer.
 */
void iobuffer_delete(iobuffer_t *const buffer)
{
    assert(buffer != NULL);

    iobuffer_free(buffer);
    free(buffer);
}

/*
 * Initialize an I/O Buffer.
 */
void iobuffer_init(iobuffer_t *const buffer, const int input_fd,
		   const int output_fd, fd_set *const read_fds,
		   fd_set *const write_fds, const char separator)
{
    assert(buffer != NULL);

    dbuffer_init(&buffer->input, input_fd, read_fds, separator);
    dbuffer_init(&buffer->output, output_fd, write_fds, separator);
}

/*
 * Free an I/O Buffer.
 */
void iobuffer_free(iobuffer_t *const buffer)
{
    assert(buffer != NULL);

    dbuffer_free(&buffer->input);
    dbuffer_free(&buffer->output);
}

/*
 * Get input buffer size.
 */
int iobuffer_get_input_size(const iobuffer_t *const buffer)
{
    assert(buffer != NULL);

    return dbuffer_get_size(&buffer->input);
}

/*
 * Get output buffer size.
 */
int iobuffer_get_output_size(const iobuffer_t *const buffer)
{
    assert(buffer != NULL);

    return dbuffer_get_size(&buffer->output);
}

/*
 * Get input file descriptor.
 */
int iobuffer_get_input_fd(const iobuffer_t *const buffer)
{
    assert(buffer != NULL);

    return dbuffer_get_fd(&buffer->input);
}

/*
 * Get output file descriptor.
 */
int iobuffer_get_output_fd(const iobuffer_t *const buffer)
{
    assert(buffer != NULL);

    return dbuffer_get_fd(&buffer->output);
}

/*
 * Get read file descriptor set.
 */
fd_set *iobuffer_get_read_fds(const iobuffer_t *const buffer)
{
    assert(buffer != NULL);

    return dbuffer_get_fds(&buffer->input);
}

/*
 * Get write file descriptor set.
 */
fd_set *iobuffer_get_write_fds(const iobuffer_t *const buffer)
{
    assert(buffer != NULL);

    return dbuffer_get_fds(&buffer->output);
}

/*
 * Get the separator character.
 */
char iobuffer_get_separator(const iobuffer_t *const buffer)
{
    assert(buffer != NULL);

    return dbuffer_get_separator(&buffer->input);
}

/*
 * Set the input file descriptor.
 */
void iobuffer_set_input_fd(iobuffer_t *const buffer, const int fd)
{
    assert(buffer != NULL);

    dbuffer_set_fd(&buffer->input, fd);
}

/*
 * Set the output file descriptor.
 */
void iobuffer_set_output_fd(iobuffer_t *const buffer, const int fd)
{
    assert(buffer != NULL);

    dbuffer_set_fd(&buffer->output, fd);
}

/*
 * Set the read file descriptor set.
 */
void iobuffer_set_read_fds(iobuffer_t *const buffer, fd_set *const fds)
{
    assert(buffer != NULL);

    dbuffer_set_fds(&buffer->input, fds);
}

/*
 * Set the write file descriptor set.
 */
void iobuffer_set_write_fds(iobuffer_t *const buffer, fd_set *const fds)
{
    assert(buffer != NULL);

    dbuffer_set_fds(&buffer->output, fds);
}

/*
 * Set the separator character.
 */
void iobuffer_set_separator(iobuffer_t *const buffer, const char separator)
{
    assert(buffer != NULL);

    dbuffer_set_separator(&buffer->input, separator);
    dbuffer_set_separator(&buffer->output, separator);
}

/*
 * Get the size of the first input token.
 */
int iobuffer_input_token_size(const iobuffer_t *const buffer)
{
    assert(buffer != NULL);

    return dbuffer_token_size(&buffer->input);
}

/*
 * Get the size of the first output token.
 */
int iobuffer_output_token_size(const iobuffer_t *const buffer)
{
    assert(buffer != NULL);

    return dbuffer_token_size(&buffer->output);
}

/*
 * Read input if it is ready.
 */
int iobuffer_read(iobuffer_t *const buffer)
{
    assert(buffer != NULL);

    return dbuffer_read(&buffer->input);
}

/*
 * Write output if it is ready.
 */
int iobuffer_write(iobuffer_t *const buffer)
{
    assert(buffer != NULL);

    return dbuffer_write(&buffer->output);
}

/*
 * Get data from the input buffer.
 */
int iobuffer_get_data(iobuffer_t *const buffer, char *const data,
		     const int data_size)
{
    assert(buffer != NULL);

    return dbuffer_get_data(&buffer->input, data, data_size);
}

/*
 * Put data to the output buffer.
 */
int iobuffer_put_data(iobuffer_t *const buffer, const char *const data,
		     const int data_size)
{
    assert(buffer != NULL);
    assert(data != NULL);

    return dbuffer_put_data(&buffer->output, data, data_size);
}

/*
 * Input a non-blank line from the input buffer.
 */
line_t *iobuffer_input_line(iobuffer_t *const buffer, const int space)
{
    assert(buffer != NULL);

    return dbuffer_input_line(&buffer->input, space);
}

/* End of file */
