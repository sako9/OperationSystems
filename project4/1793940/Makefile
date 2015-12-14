CC = gcc# compiler
CFLAGS = -Wall -g# compile flags
LIBS = -lm# libs

all: disksched

disksched: disksched.o
	$(CC) -o disksched disksched.o $(LIBS)

%.o:%.c
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm -f disksched *.o *~
