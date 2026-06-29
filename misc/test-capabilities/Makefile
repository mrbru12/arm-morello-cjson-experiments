CC = clang-morello
CFLAGS = -Wall -Wextra -pedantic -march=morello -mabi=purecap
LDFLAGS =

all: bin src/main.c
	$(CC) src/main.c $(CFLAGS) $(LDFLAGS) -o bin/main

bin:
	mkdir bin

clean:
	rm -r bin

.PHONY: all 
