# ----------------------------------------------------------------------------
#
# Minitalk: a basic talk-like server/client
# Copyright (C) 2004 Benjamin Gaillard
#
# ----------------------------------------------------------------------------
#
#        File: server/GNUmakefile
#
# Description: Server Make File
#
#     Comment: Use `make' to complie, `make depend' to update the dependencies
#              in make.dep and  `make clean' to remove the object files and
#              the executable file.
#              Warning!  Launch `make clean' before compiling the program on
#              another architecture.
#
# ----------------------------------------------------------------------------
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc., 59
# Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# ----------------------------------------------------------------------------


# Global variables
TOPDIR   = ..
EXE      = mtserver
INCLUDES = -I../config -I../strlib

# Make rules
include ../config/rules.mk

# Explicit dependencies
mtserver: LIBS += -L../strlib -lmtstr
mtserver: ../strlib/libmtstr.a

# End of file
