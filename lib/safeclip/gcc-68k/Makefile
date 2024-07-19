# Target: demo

# Flags
CC = gcc
CFLAGS = -O -m68020 -noixemul
LDFLAGS = -L. -s

# Objects
OBJ = demo.o safeclip.o clip.o

.s.o: 
	gasp $*.s > t.s
	as -o $*.o t.s
	rm -f t.s

demo: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ)

$(OBJ): safeclip.h




