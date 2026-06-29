gC = clang-morello
CFLAGS = -Wall -Wextra -pedantic
LDFLAGS =

all: bin src/main.c
	$(CC) src/main.c $(CFLAGS) $(LDFLAGS) -o bin/gen_json

bin:
	mkdir bin

clean:
	rm -r bin

.PHONY: all, clean 
