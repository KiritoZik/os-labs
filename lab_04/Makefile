CC = gcc
CFLAGS = -D_POSIX_C_SOURCE=200809L -std=c17 -O2 -Wall -Wextra -Wpedantic -pthread
TARGET = $(HOME)/lab04

.PHONY: all run clean

all: $(TARGET)

$(TARGET): src/main.c src/calculator.c src/calculator.h
	$(CC) $(CFLAGS) src/main.c src/calculator.c -o $(TARGET)

run: all
	cd $(HOME) && ./lab04

clean:
	rm -f $(TARGET)
