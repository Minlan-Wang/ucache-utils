#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include "bcache.h"

int main(int argc, char *argv[])
{
	struct bkey key;

	if (argc < 4)
		return 0;

	sscanf(argv[1], "0x%llx", &(key.high));
	sscanf(argv[2], "0x%llx", &(key.low));
	sscanf(argv[3], "0x%llx", &(key.ptr));

	printf("0x%llx, 0x%llx, 0x%llx\n", key.high, key.low, key.ptr);
	printf("############################\n");
	dump_bkey(&key);
	printf("############################\n");

	return 0;
}
