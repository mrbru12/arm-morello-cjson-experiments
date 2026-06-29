#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

#define JSON_PATH "/home/regis/TCC-Bruno/test-cjson/res/512KB.json"

char *read_whole_file(const char *path) {
	FILE *file;
	long size;
	char *buffer, temp_buffer[32];
       
	file = fopen(path, "rb");

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	rewind(file);

	buffer = calloc(1, size + 1);

	while (fgets(temp_buffer, sizeof(temp_buffer), file)) {
		strcat(buffer, temp_buffer);
	}

	fclose(file);

	return buffer;
}

int main(int argc, char *argv[]) {
	char *json_src, *json_str;
	cJSON *json;

	(void)argc; (void)argv;

	json_src = read_whole_file(JSON_PATH);
	printf("1. File read!\n");

	json = cJSON_Parse(json_src);
	printf("2. JSON parsed!\n");

	json_str = cJSON_Print(json);
	printf("3. JSON printted!\n");

	/* printf("=====\n%s\n=====\n", json_str); */
	free(json_str);

	cJSON_Delete(json);
	free(json_src);

	printf("4. End!\n");

	return 0;
}
