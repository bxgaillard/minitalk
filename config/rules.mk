# ----------------------------------------------------------------------------
#
# Minitalk: a basic talk-like server/client
# Copyright (C) 2004 Benjamin Gaillard
#
# ----------------------------------------------------------------------------
#
#        File: config/rules.mk
#
# Description: Make Rules
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


##############################################################################
#
# Configuration parameters
#
#

# Programs and flags
ifeq ($(TOPDIR),)
    include config/flags.mk
else
    include $(TOPDIR)/config/flags.mk
endif


##############################################################################
#
# Global files and flags
#
#

# Current working directory
CWD := $(shell pwd)

# Headers and libraries directories
INCLUDES ?=
LIBS     ?=

# Source and object files
SRC := $(strip $(SRC) $(wildcard *.c))
HDR := $(strip $(HDR) $(wildcard *.h))
OBJ := $(strip $(OBJ) $(SRC:.c=.o))

# Resulting program
ifeq ($(EXE),)
    ifeq ($(LIB),)
        ifneq ($(SRC),)
            EXE := $(notdir $CWD)
        endif
    endif
endif

# Makefile and file containing dependencies names
MKFNAMES = Makefile makefile GNUmakefile
MAKEFILE := $(firstword $(foreach file,$(MKFNAMES),$(wildcard $(file))))
ifneq ($(TOPDIR),)
    RULEFILE := $(TOPDIR)/config/rules.mk $(TOPDIR)/config/flags.mk
endif
ifneq ($(SRC),)
    DEPFILE     := make.dep
    REALDEPFILE := $(wildcard $(DEPFILE))
endif

# Subdirectories containing makefiles
SUBDIRS := $(SUBDIRS) $(sort $(foreach file,$(MKFNAMES),$(subst /$(file),, \
				       $(wildcard */$(file)))))


##############################################################################
#
# Special rules
#
#

# Silent execution (without echoing)
.SILENT:

# User file suffixes
.SUFFIXES:
.SUFFIXES: .c .o .a

# Rules not generating files
.PHONY: default final debug all infos clean run depend depclean $(SUBDIRS)


##############################################################################
#
# Generic rules
#
#

# Compilation of a C source file
%.o: %.c $(MAKEFILE) $(RULEFILE)
	echo "Compiling \`$<'..."
	$(CC) $(CFLAGS) $(WARN) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

# Build a library in another directory
%.a: $(RULEFILE)
	echo "** Building \`$@'; entering directory $(dir $@) **"
	$(MAKE) -C $(dir $@) $(TARGET)
	echo "** Building of \`$@' done; leaving directory $(dir $@) **"


##############################################################################
#
# General rules
#
#

# By default, compile everything
default: final
#default: debug

# Compilation without debug informations
final: TARGET    = final
final: CPPFLAGS += -DNDEBUG
final: all

# Compilation with debug informations
debug: TARGET    = debug
debug: CFLAGS   := $(DEBUG)
debug: CPPFLAGS += -DDEBUG
debug: LDFLAGS  := $(filter-out -s,$(LDFLAGS))
debug: all

# Compilation with extensive debug information (e.g. memory allocation)
harddebug: TARGET = harddebug
harddebug: CPPFLAGS += -DHARDDEBUG
harddebug: debug

# Explicit dependencies
ifneq ($(REALDEPFILE),)
    include $(DEPFILE)
endif

# Compile everything
all: $(SUBDIRS) depend $(LIB) $(EXE)

# Display compilation informations
info infos:
	echo 'C compiler:         $(CC)'
	echo 'C flags:            $(CFLAGS)'
	echo 'Warning flags:      $(WARN)'
	echo 'Preprocessor flags: $(CPPFLAGS)'
	echo 'Linker flags:       $(LDFLAGS)'

# Link the executable file
ifneq ($(EXE),)
    $(EXE): $(OBJ) $(MAKEFILE) $(RULEFILE) $(DEPFILE)
	echo "Linking \`$@'..."
	$(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o $@
endif

# Link a static library
ifneq ($(LIB),)
    $(LIB):
	echo "Creating \`$@'..."
	$(AR) cru $@ $^
	$(RANLIB) $@
endif

# Subdirectories
$(SUBDIRS):
	echo '** Entering directory $(CWD)/$@ **'
	$(MAKE) -C $@ $(TARGET)
	echo '** Leaving directory $(CWD)/$@ **'

# Remove object and executable files
clean: TARGET = clean
clean: $(SUBDIRS) depclean
	echo 'Cleaning up directory...'
	$(RM) $(OBJ) $(LIB) $(EXE) $(DEPFILE).bak *~ \#*\#
	$(RM) core $(addsuffix .core,$(EXE))

# Launch the program
run: default
	echo "Executing \`$(EXE)':"
	./$(EXE)


##############################################################################
#
# Dependencies management
#
#

# Create dependency file

# Update dependencies
depend:     $(DEPFILE)
$(DEPFILE): $(SRC) $(HDR)
	echo 'Updating dependencies ($@)...'
ifeq ($(REALDEPFILE),)
	echo 'AUTODEPFILE = 1' > $(DEPFILE)
	echo '# DO NOT DELETE' >> $(DEPFILE)
	echo 'CLEANDEP = 1' >> $(DEPFILE)
endif
	$(MKDEP) -f $@ -- $(CPPFLAGS) $(INCLUDES) -- $(SRC) 2>/dev/null
	$(RM) $(DEPFILE).bak

# Reset dependencies
depclean:
ifeq ($(AUTODEPFILE),1)
	echo 'Deleting dependency file...'
	$(RM) $(DEPFILE)
else
    ifneq ($(REALDEPFILE),)
	echo 'Reseting dependencies ($(DEPFILE))...'
	head -n `grep 'DO NOT DELETE' -m 1 -n $(DEPFILE) | cut -f1 -d:` \
	    $(DEPFILE) > $(DEPFILE).tmp
	$(MV) $(DEPFILE).tmp $(DEPFILE)
	echo >> $(DEPFILE)
	echo '# Empty dependencies' >> $(DEPFILE)
	echo 'CLEANDEP = 1' >> $(DEPFILE)
    endif
endif

# End of file
