#include <stdio.h>

int main(int argc, char *argv[]) {
	(void)argc; (void)argv;

	int value = 64;
	size_t capability_size = sizeof(&value);

	printf("Size of capability:\n");
	printf("\t- %lu bytes\n", capability_size);
	printf("\t- %lu bits\n", capability_size * 8);

	return 0;
}
