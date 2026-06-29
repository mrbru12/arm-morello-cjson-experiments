CC = clang-morello
CFLAGS = -Wall -Wextra -pedantic
CAPFLAGS = -mabi=purecap -march=morello
WRAPFLAGS = -Wl,--wrap=malloc,--wrap=realloc,--wrap=calloc,--wrap=free
LDFLAGS = -lm

all: bin src/main.c src/cJSON.c src/cJSON.h
	$(CC) src/main.c src/cJSON.c $(CFLAGS) $(CAPFLAGS) $(WRAPFLAGS) $(LDFLAGS) -o bin/backend_purecap
	$(CC) src/main.c -L../../cJSON_shared/lib -lcjson_purecap -Wl,-rpath,/home/regis/TCC-Bruno/cJSON_shared/lib $(CFLAGS) $(CAPFLAGS) $(WRAPFLAGS) $(LDFLAGS) -o bin/backend_purecap_shared
	$(CC) src/main.c src/cJSON.c $(CFLAGS) $(WRAPFLAGS) $(LDFLAGS) -o bin/backend

bin:
	mkdir bin

clean:
	rm -r bin

.PHONY: all, clean 
