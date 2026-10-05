CC = gcc
CPPFLAGS = -D_POSIX_C_SOURCE=200809L
CFLAGS = -std=c17 -Wall -Wextra -Wpedantic -g

.PHONY: all run clean

all: parent child

parent: parent.c
	$(CC) $(CPPFLAGS) $(CFLAGS) parent.c -o parent

child: child.c
	$(CC) $(CPPFLAGS) $(CFLAGS) child.c -o child

run: all
	./parent

clean:
	rm -f parent child