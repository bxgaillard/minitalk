/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: strlib/iobuffer.h
 *
 * Description: Dynamic Input/Output Buffers (Header)
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


#ifndef IOBUFFER_H
#define IOBUFFER_H


/*
 * Headers
 */

/* System headers */
#include <sys/select.h> /* fd_set */

/* Project headers */
#include <dbuffer.h> /* dbuffer_t, line_t */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*
 * Data types
 */

/* Dynamic input/output buffer */
typedef struct iobuffer {
    dbuffer_t input;  /* Input dynamic buffer  */
    dbuffer_t output; /* Output dynamic buffer */
} iobuffer_t;


/*
 * Prototypes
 */

/* Constructors and destructors */
iobuffer_t *iobuffer_new(const int input_fd, const int output_fd,
			 fd_set *const read_fds, fd_set *const write_fds,
			 const char separator);
void        iobuffer_delete(iobuffer_t *const buffer);
void        iobuffer_init(iobuffer_t *const buffer, const int input_fd,
			  const int output_fd, fd_set *const read_fds,
			  fd_set *const write_fds, const char separator);
void        iobuffer_free(iobuffer_t *const buffer);

/* Accessors */
int     iobuffer_get_input_size(const iobuffer_t *const buffer);
int     iobuffer_get_output_size(const iobuffer_t *const buffer);
int     iobuffer_get_input_fd(const iobuffer_t *const buffer);
int     iobuffer_get_output_fd(const iobuffer_t *const buffer);
fd_set *iobuffer_get_read_fds(const iobuffer_t *const buffer);
fd_set *iobuffer_get_write_fds(const iobuffer_t *const buffer);
char    iobuffer_get_separator(const iobuffer_t *const buffer);
void    iobuffer_set_input_fd(iobuffer_t *const buffer, const int fd);
void    iobuffer_set_output_fd(iobuffer_t *const buffer, const int fd);
void    iobuffer_set_read_fds(iobuffer_t *const buffer, fd_set *const fds);
void    iobuffer_set_write_fds(iobuffer_t *const buffer, fd_set *const fds);
void    iobuffer_set_separator(iobuffer_t *const buffer,
			       const char separator);
/* Methods */
int     iobuffer_input_token_size(const iobuffer_t *const buffer);
int     iobuffer_output_token_size(const iobuffer_t *const buffer);
int     iobuffer_read(iobuffer_t *const buffer);
int     iobuffer_write(iobuffer_t *const buffer);
int     iobuffer_get_data(iobuffer_t *const buffer, char *const data,
			  const int data_size);
int     iobuffer_put_data(iobuffer_t *const buffer, const char *const data,
			  const int data_size);
line_t *iobuffer_input_line(iobuffer_t *const buffer, const int space);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !IOBUFFER_H */

/* End of file */
