/*
 * ---------------------------------------------------------------------------
 *
 * Minitalk: a basic talk-like server/client
 * Copyright (C) 2004 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: strlib/dbuffer.h
 *
 * Description: Dynamic Buffers (Header)
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


#ifndef DBUFFER_H
#define DBUFFER_H

/* System headers */
#include <sys/select.h> /* fd_set */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*
 * Data types
 */

/* Dynamic buffer */
typedef struct dbuffer {
    struct ibuffer *first;     /* First element in linked list             */
    struct ibuffer *last;      /* Last element in linked list              */
    int             size;      /* Total size of data in the buffer         */
    int             fd;        /* File/socket descriptor to read data from */
    fd_set         *fds;       /* Read/write descriptor set                */
    char            separator; /* Character separating tokens              */
} dbuffer_t;

/* Input line */
typedef struct line {
    int   length; /* Line length                                     */
    char *start;  /* Pointer to the data beginning (before the line) */
    char *data;   /* Line data                                       */
} line_t;


/*
 * Prototypes
 */

/* Constructors and destructors */
dbuffer_t *dbuffer_new(const int fd, fd_set *const fds, const char separator);
void       dbuffer_delete(dbuffer_t *const buffer);
void       dbuffer_init(dbuffer_t *const buffer, const int fd,
			fd_set *const fds, const char separator);
void       dbuffer_free(dbuffer_t *const buffer);

/* Accessors */
int     dbuffer_get_size(const dbuffer_t *const buffer);
int     dbuffer_get_fd(const dbuffer_t *const buffer);
fd_set *dbuffer_get_fds(const dbuffer_t *const buffer);
char    dbuffer_get_separator(const dbuffer_t *const buffer);
void    dbuffer_set_fd(dbuffer_t *const buffer, const int fd);
void    dbuffer_set_fds(dbuffer_t *const buffer, fd_set *const fds);
void    dbuffer_set_separator(dbuffer_t *const buffer, const char separator);

/* Methods */
int     dbuffer_read(dbuffer_t *const buffer);
int     dbuffer_write(dbuffer_t *const buffer);
int     dbuffer_token_size(const dbuffer_t *const buffer);
int     dbuffer_get_data(dbuffer_t *const buffer, char *const data,
			 const int data_size);
int     dbuffer_put_data(dbuffer_t *const buffer, const char *const data,
			 const int data_size);
line_t *dbuffer_input_line(dbuffer_t *const buffer, const int space);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !DBUFFER_H */

/* End of file */
