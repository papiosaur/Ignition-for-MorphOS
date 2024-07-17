## ignition-Makefile
##
## Copyright 1996-2009 pinc Software. All Rights Reserved.
## Licensed under the terms of the GNU General Public License, version 3.


###########################################################################
# I've borrowed some ideas from the Makefile for "codesets.library".
# Note that this works currently only for AROS !


###########################################################################
# This makefile is a very generic one. It tries to identify both, the host
# and the target operating system for which Ignition should be compiled.
# However, this auto-detection can be easily overridden by directly
# specifying an option on the commandline while calling 'make'.
#
# Example:
#
# # to explicitly compile for AmigaOS3
# > make OS=os3
#
# # to compile for AmigaOS4 but without debugging
# > make OS=os4 DEBUG=
#

#############################################
# find out the HOST operating system
# on which this makefile is run
HOST ?= $(shell uname)
ifeq ($(HOST), AmigaOS)
  ifeq ($(shell uname -m), powerpc)
    HOST = AmigaOS4
  endif
  ifeq ($(shell uname -m), ppc)
    HOST = AmigaOS4
  endif
endif

# if no host is identifed (no uname tool)
# we assume a AmigaOS build
ifeq ($(HOST),)
  HOST = AmigaOS
endif

#############################################
# now we find out the target OS for
# which we are going to compile Ignition in case
# the caller didn't yet define OS himself
ifndef (OS)
  ifeq ($(HOST), AmigaOS4)
    OS = os4
  else
  ifeq ($(HOST), AmigaOS)
    OS = os3
  else
  ifeq ($(HOST), MorphOS)
    OS = mos
  else
  ifeq ($(HOST), AROS)
    # now we find out which CPU system aros will be used
    ifeq ($(shell uname -m), powerpc)
      OS = aros-ppc
    endif
	 ifeq ($(shell uname -m), ppc)
      OS = aros-ppc
    endif
	 ifeq ($(shell uname -m), i386)
      OS = aros-i386
    endif
	 ifeq ($(shell uname -m), i686)
      OS = aros-i686
    endif
	 ifeq ($(shell uname -m), x86_64)
      OS = aros-x86_64
    endif
  else
    OS = os4
  endif
  endif
  endif
  endif
endif

#############################################
# define common commands we use in this
# makefile. Please note that each of them
# might be overridden on the commandline.

# common commands
FLEX    = flex
FC      = FlexCat
EXPR    = expr
DATE    = date
RM      = delete force
RMDIR   = delete force all
MKDIR   = makedir
CHMOD   = protect FLAGS=rwed
CP      = copy
CC      = gcc
STRIP   = strip
OBJDUMP = objdump

# path definitions
CDUP  = /
CDTHIS=

# override some variables for non-native builds (cross-compiler)
ifneq ($(HOST), AmigaOS)
ifneq ($(HOST), AmigaOS4)
ifneq ($(HOST), MorphOS)

  # when we end up here this is either a unix or Aros host
  # so lets use unix kind of commands
  RM    = rm -f
  RMDIR = rm -rf
  MKDIR = mkdir -p
  CHMOD = chmod 755
  CP    = cp -f

  CDUP  = ../
  CDTHIS= ./

endif
endif
endif

###########################################################################
# CPU and DEBUG can be defined outside, defaults to above
# using e.g. "make DEBUG= CPU=-mcpu=603e" produces optimized non-debug
# PPC-603e version
#
# OPTFLAGS are disabled by DEBUG normally!
#
# ignored warnings are:
# none - because we want to compile with -Wall all the time

# Common Directories
PREFIX   = $(CDTHIS)
OBJDIR   = OBJS
VPATH    = $(OBJDIR)

# Common compiler/linker flags
WARN     = -W -Wall
OPTFLAGS = -O2 -fomit-frame-pointer -funroll-loops
DEBUG    = -DDEBUG -O0
DEBUGSYM = -gstabs
CFLAGS   = -I.. -I../libs/include -I../libs/gtdrag/include \
           $(CPU) $(WARN) $(OPTFLAGS) $(DEBUG) $(DEBUGSYM) $(USER_CFLAGS)
LDFLAGS  = $(CPU) $(DEBUGSYM) -nostartfiles
LDLIBS   =

# different options per target OS
ifeq ($(OS), os4)

  ##############################
  # AmigaOS4

  # Compiler/link/strip commands
  ifneq ($(HOST), AmigaOS4)
    CC      = ppc-amigaos-gcc
    STRIP   = ppc-amigaos-strip
    OBJDUMP = ppc-amigaos-objdump
  endif

  # Compiler/Linker flags
#  CRT      = newlib
#  CPU      = -mcpu=powerpc
#  WARN     += -Wdeclaration-after-statement -Wdisabled-optimization
#  REDEFINE =
#  CFLAGS   += -mcrt=$(CRT) -D__USE_INLINE__ -D__NEW_TIMEVAL_DEFINITION_USED__ \
#              $(REDEFINE) -Wa,-mregnames
#  LDFLAGS  += -mcrt=$(CRT)
   CFLAGS = -D__USE_INLINE__ -nostdlib -O3 -gstabs -I../libs/gtdrag-OS4/include
   LDFLAGS = -D__USE_INLINE__ -nostdlib -O3 -gstabs
else
ifeq ($(OS), os3)

  ##############################
  # AmigaOS3

  # Compiler/link/strip commands
  ifneq ($(HOST), AmigaOS)
    CC      = m68k-amigaos-gcc
    STRIP   = m68k-amigaos-strip
    OBJDUMP = m68k-amigaos-objdump
  endif

  # Compiler/Linker flags
  CPU      = -m68020-60 -msoft-float
  CFLAGS  += -noixemul
  LDFLAGS += -noixemul
  LDLIBS  += -ldebug

else
ifeq ($(OS), mos)

  ##############################
  # MorphOS

  # Compiler/link/strip commands
  ifneq ($(HOST), MorphOS)
    CC      = ppc-morphos-gcc
    STRIP   = ppc-morphos-strip
    OBJDUMP = ppc-morphos-objdump
  endif

  # Compiler/Linker flags
  CPU     = -mcpu=powerpc
  CFLAGS  += -noixemul
  LDFLAGS += -noixemul
  LDLIBS  += -ldebug

  OBJS = stubs-morphos.o

else
ifeq ($(OS), aros-i386)

  ##############################
  # AROS (i386)

  ifneq ($(HOST), AROS)
    CC      = i386-aros-gcc
    STRIP   = i386-aros-strip
    OBJDUMP = i386-aros-objdump
  endif

  # Compiler/Linker flags
  CFLAGS += -Wno-pointer-sign -UDEBUG -fno-stack-protector
  OBJS   += 
  LDLIBS += -lrom

else
ifeq ($(OS), aros-ppc)

  ##############################
  # AROS (PPC)

  ifneq ($(HOST), AROS)
    CC      = ppc-aros-gcc
    STRIP   = ppc-aros-strip
    OBJDUMP = ppc-aros-objdump
  endif

  # Compiler/Linker flags
  CFLAGS += -Wno-pointer-sign
  LDLIBS +=

else
ifeq ($(OS), aros-x86_64)

  ##############################
  # AROS (x86_64)

  ifneq ($(HOST), AROS)
    CC      = x86_64-aros-gcc
    STRIP   = x86_64-aros-strip
    OBJDUMP = x86_64-aros-objdump
  endif

  # Compiler/Linker flags
  CFLAGS += -Wno-pointer-sign
  LDLIBS +=

endif
endif
endif
endif
endif
endif

###########################################################################
# Here starts all stuff that is common for all target platforms and
# hosts.

IOOBJS			= $(OBJDIR)/iotype.o $(OBJDIR)/iotype_variadic.o
GCOBJS			= $(OBJDIR)/gclass.o $(OBJDIR)/gclass_variadic.o

# main target
.PHONY: all
all: $(OBJDIR) ioobjs iotypes gcobjs gclasses

# make the object directories
$(OBJDIR):
	@echo "  MK $@"
	@$(MKDIR) $(OBJDIR)

# for compiling single .c files
$(OBJDIR)/%.o: %.c
	@echo "  CC $<"
	$(CC) $(CFLAGS) -c $< -o $@

%.io : %.o $(IOOBJS)
	@echo "  LD $@"
	@$(CC) $(LDFLAGS) -o $@ $(IOOBJS) $< $(LDLIBS)
	@$(CHMOD) $@

%.gc : %.o $(GCOBJS)
	@echo "  LD $@"
	@$(CC) $(LDFLAGS) -o $@ $(GCOBJS) $< $(LDLIBS)
	@$(CHMOD) $@

# cleanup target
.PHONY: clean
clean:
	-$(RM) *.gc *.io $(OBJDIR)/*.o

# clean all stuff, including our autotools
.PHONY: distclean
distclean: clean
	-$(RMDIR) $(OBJDIR)

.PHONY: ioobjs iotypes gcobjs gclasses

ioobjs:			$(IOOBJS) $(OBJDIR)/csv.o $(OBJDIR)/ascii.o $(OBJDIR)/turbocalc.o $(OBJDIR)/ods.o
iotypes:		csv.io ascii.io turbocalc.io ods.io

gcobjs:			$(GCOBJS) \
				$(OBJDIR)/button.o $(OBJDIR)/checkbox.o $(OBJDIR)/frame.o $(OBJDIR)/text.o \
				$(OBJDIR)/line.o $(OBJDIR)/rectangle.o $(OBJDIR)/gradient.o \
				$(OBJDIR)/area_diagram.o $(OBJDIR)/column_diagram.o $(OBJDIR)/line_diagram.o $(OBJDIR)/circle_diagram.o

gclasses:		button.gc checkbox.gc frame.gc text.gc \
				line.gc rectangle.gc gradient.gc \
				area_diagram.gc column_diagram.gc line_diagram.gc circle_diagram.gc

iotype.c:		iotype.h
gclass.c:		gclass.h
