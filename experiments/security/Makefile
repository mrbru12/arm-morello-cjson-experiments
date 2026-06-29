CC       = clang-morello
CFLAGS   = -Wall -Wextra -g
CAPFLAGS = -march=morello -mabi=purecap

all: bin bin/vuln_baseline bin/vuln_purecap

bin:
	mkdir -p bin

bin/vuln_baseline: src/vuln_consumer.c src/cJSON.c src/cJSON.h
	$(CC) src/vuln_consumer.c src/cJSON.c $(CFLAGS) -o bin/vuln_baseline

bin/vuln_purecap: src/vuln_consumer.c src/cJSON.c src/cJSON.h
	$(CC) src/vuln_consumer.c src/cJSON.c $(CFLAGS) $(CAPFLAGS) -o bin/vuln_purecap

clean:
	rm -rf bin

.PHONY: all clean
