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

# target definition
TARGET   = Ignition

# Common compiler/linker flags
WARN     = -W -Wall
OPTFLAGS = -O2 -fomit-frame-pointer -funroll-loops
DEBUG    = -DDEBUG -O0
DEBUGSYM = -gstabs
CFLAGS   = -Ilibs -Ilibs/include \
            $(CPU) $(WARN) $(OPTFLAGS) $(DEBUG) $(DEBUGSYM) $(USER_CFLAGS)
LDFLAGS  = $(CPU) $(DEBUGSYM)
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
  CRT      = newlib
  CPU      = -mcpu=powerpc
  WARN     += -Wdeclaration-after-statement -Wdisabled-optimization
  REDEFINE =
  CFLAGS   += -mcrt=$(CRT) -D__USE_INLINE__ -D__NEW_TIMEVAL_DEFINITION_USED__ -D__AMIGAOS41__\
              $(REDEFINE) -Wa,-mregnames
  LDFLAGS  += -mcrt=$(CRT)

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

  #OBJS = stubs-morphos.o  /line deactivated because file not present, author contacted to ask why :-) 

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
  CFLAGS += -Ilib/safeclip/generic \
            -Ilibs/gtdrag/aros/include -Ilibs/gtdrag/include \
            -Ilibs/scroller/aros/include -Ilibs/scroller/include \
            -Ilibs/textedit/aros/include -Ilibs/textedit/include \
            -Wno-pointer-sign -UDEBUG -fno-stack-protector
  OBJS   += lib/safeclip/generic/safeclip.o
  LDLIBS += -lrexxsyslib -lmathieeedoubbas -lmathieeedoubtrans

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

OBJS +=	ignition.o prefs.o prefsio.o edit.o table.o cell.o calc.o font.o \
		functions.o handlewindows.o gadgets.o initwindows.o images.o windows.o \
		objects.o debug.o rexx.o support.o project.o io.o classes.o \
		handleprefs.o color.o reference.o prefsgadgets.o graphic.o clip.o \
		undo.o hooks.o database.o pointer.o boopsi.o \
		diagram.o printer.o lock.o cmd.o menu.o popper.o screen.o search.o \
		compatibility.o locale.o

# main target
.PHONY: all
all: $(OBJDIR) $(TARGET) addons

.PHONY: addons
addons:
	$(MAKE) -C add-ons

# make the object directories
$(OBJDIR):
	@echo "  MK $@"
	@$(MKDIR) $(OBJDIR)
	@$(MKDIR) $(OBJDIR)/libs
	@$(MKDIR) $(OBJDIR)/lib/safeclip/generic

# for compiling single .c files
%.o: %.c
	@echo "  CC $<"
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@

ignition_strings.h: locale/ignition.cd
	$(FC) $< $@=locale/C_h_orig.sd

# for linking the target
$(TARGET): $(OBJS)
	@echo "  LD $@.debug"
	@$(CC) $(LDFLAGS) -o $@.debug $(addprefix $(OBJDIR)/,$(OBJS)) $(LDLIBS)
	@echo "  LD $@"
	@$(STRIP) --strip-unneeded --remove-section .comment -o $@ $@.debug
	@$(CHMOD) $@

# cleanup target
.PHONY: clean
clean:
	-$(RM) $(TARGET) $(TARGET).debug $(OBJDIR)/*.o
	$(MAKE) clean -C add-ons

# clean all including .obj directory
.PHONY: distclean
distclean: clean
	-$(RMDIR) $(OBJDIR)
	-$(RM) ignition_strings.h Makefile.dep
	$(MAKE) distclean -C add-ons

## DEPENDENCY GENERATION ##############

Makefile.dep: ;
	@echo "WARNING: Makefile.dep missing. Please run 'make depend'"

.PHONY: depend
depend: ignition_strings.h
	@echo "  MK Makefile.dep"
	@echo "# AUTOGENERATED! DO NOT EDIT!!!" >Makefile.dep
	@$(CC) -MM $(CFLAGS) $(patsubst %.o,%.c, $(OBJS)) >>Makefile.dep
	@echo "# AUTOGENERATED! DO NOT EDIT!!!" >>Makefile.dep

## DEPENDENCY INCLUDE #################

-include Makefile.dep
