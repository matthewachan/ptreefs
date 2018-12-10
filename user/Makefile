CC = gcc
CFLAGS = -g -O2 -Wall

.PHONY: all clean

all: ptreeps

test: ptreeps.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f ptreeps
