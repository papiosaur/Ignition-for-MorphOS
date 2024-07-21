# Copyright ©1999-2010 pinc Software. All Rights Reserved.
# Licensed under the terms of the MIT License.

CC = i386-aros-gcc
CFLAGS = -Wall -Wno-pointer-sign -I../../include -Iinclude \
	-D__GRAPHICS_NOLIBBASE__ -D__INTUITION_NOLIBBASE__ -D__UTILITY_NOLIBBASE__ \
	-D__DOS_NOLIBBASE__ -D__IFFPARSE_NOLIBBASE__
LDFLAGS = -lrom -nostartfiles

EXE = ../pTextEdit.gadget

OBJS = TextEdit_lib.o TE_clip.o TE_dispatch.o TE_draw.o TextEdit.o


.PHONY: all
all : $(EXE)

$(EXE) : $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

.PHONY: clean
clean :
	rm -f $(OBJS) $(EXE)

.PHONY: distclean
distclean : clean
