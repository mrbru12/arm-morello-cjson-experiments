CC = clang-morello
CFLAGS = -Wall -Wextra -pedantic
LDFLAGS =

all: bin src/main.c src/cJSON.c src/cJSON.h
	$(CC) src/main.c src/cJSON.c $(CFLAGS) $(LDFLAGS) -o bin/gateway

bin:
	mkdir bin

clean:
	rm -r bin

.PHONY: all, clean 
