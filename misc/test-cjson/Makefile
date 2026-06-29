CC = clang-morello
CFLAGS = -Wall -Wextra -pedantic -std=c89 -march=morello -mabi=purecap
LDFLAGS =

all: bin src/main.c src/cJSON.c src/cJSON.h
	$(CC) src/main.c src/cJSON.c $(CFLAGS) $(LDFLAGS) -o bin/main

bin:
	mkdir bin

clean:
	rm -r bin

.PHONY: all 
