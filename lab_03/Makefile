CC = gcc
CFLAGS = -D_POSIX_C_SOURCE=200809L -std=c17 -O2 -Wall -Wextra -Wpedantic -pthread
TARGET = $(HOME)/sort

.PHONY: all run clean

all: $(TARGET)

$(TARGET): src/main.c src/batcher.c src/batcher.h
	$(CC) $(CFLAGS) src/main.c src/batcher.c -o $(TARGET)

run: all
	$(TARGET) -n 100000 -t 4

clean:
	rm -f $(TARGET)