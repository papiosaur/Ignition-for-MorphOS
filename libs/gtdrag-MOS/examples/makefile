# Makefile for examples
# 
# 

# Uncomment the next line if you are cross compiling
# CROSS_COMPILE = ppc-amigaos-

CC     = $(CROSS_COMPILE)gcc 
CXX    = $(CROSS_COMPILE)c++ 
# Makefile for examples
# 
# 

# Uncomment the next line if you are cross compiling
# CROSS_COMPILE = ppc-amigaos-

CC     = $(CROSS_COMPILE)gcc 
CXX    = $(CROSS_COMPILE)c++ 
AS     = $(CROSS_COMPILE)as 
LD     = $(CROSS_COMPILE)ld 
RANLIB = $(CROSS_COMPILE)ranlib 
RM     = delete
# RM     = rm

# Change these as required
OPTIMIZE = -O3
DEBUG    = 
#DEBUG = -gstabs -DDEBUG
CFLAGS   =  -D__USE_INLINE__ -Wall $(OPTIMIZE) $(DEBUG) -I. -I../include 

# Flags passed to gcc during linking
LINK =

# Name of the "thing" to build
TARGETS = dragtest boopsi OeffneInterface

# Additional linker libraries
LIBS = -lauto -lgtdrag_auto

#
# -------------------------------------------------------------
# Nothing should need changing below this line

make_all: $(TARGETS)
	

# Rules for building
dragtest: 
	$(CC) $(CFLAGS)  -o dragtest dragtest.c $(LIBS)

boopsi: 
	$(CC) $(CFLAGS)  -o boopsi boopsi.c $(LIBS)

OeffneInterface:
	$(CC) $(CFLAGS) -o OeffneInterface  OeffneInterface.c $(LIBS)
