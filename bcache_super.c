#include <stdio.h> 
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <uuid/uuid.h>
#include <string.h>
#include "bcache.h"
#include "list.h"

/*
 * size_t: unsigned long
 * off_t: unsigned long
 * ssize_t: unsigned int
 */

void help_info(void)
{
	printf("./bcache_super CACHE_DEV\n");
}

void dump_sb(struct cache_sb *sb)
{
	char uuid[40] = {0};
	uuid_unparse(sb->uuid, uuid);
	printf("uuid:\t\t\t%s\n", uuid);

	uuid_unparse(sb->set_uuid, uuid);
	printf("set uuid:\t\t%s\n", uuid);

	printf("offset:\t\t\t%lu\n", sb->offset);
	printf("version:\t\t%lu\n", sb->version);
	printf("nbuckets:\t\t%lu\n", sb->nbuckets);
	printf("block_size:\t\t%u%s\n", sb->block_size >> (KB_BITS - SECTOR_BITS), "KB");
	printf("bucket_size:\t\t%u%s\n", sb->bucket_size >> (KB_BITS - SECTOR_BITS), "KB");
	printf("nr_in_set:\t\t%u\n", sb->nr_in_set);
	printf("nr_this_dev:\t\t%u\n", sb->nr_this_dev);
	printf("first_bucket:\t\t%u\n", sb->first_bucket);
	printf("journal_buckets:\t%u\n", sb->njournal_buckets);
	switch(CACHE_REPLACEMENT(sb)) {
		case CACHE_REPLACEMENT_LRU:
			printf("replacement:\t\t%s\n", "LRU");
			break;
		case CACHE_REPLACEMENT_FIFO:
			printf("replacement:\t\t%s\n", "FIFO");
			break;
		case CACHE_REPLACEMENT_RANDOM:
			printf("replacement:\t\t%s\n", "RANDOM");
			break;
	}
}

int read_sb(int fd, struct cache_sb *sb)
{
	int ret;
	
	ret = pread(fd, sb, sizeof(struct cache_sb), SB_START);
	if (ret != sizeof(struct cache_sb)) {
		fprintf(stderr, "bcache super block read failed.\n");
		return -1;
	}

	if (memcmp(sb->magic, bcache_magic, 16)) {
		fprintf(stderr, "magic varify failed.\n");
		return -1;
	}

	if (sb->offset != SB_SECTOR) {
		fprintf(stderr, "sb offset verify failed.\n");
		return -1;
	}

	if (sb->csum != csum_set(sb)) {
		fprintf(stderr, "csum verify failed.\n");
		return -1;
	}

	if (sb->version != BCACHE_SB_VERSION_CDEV &&
		sb->version != BCACHE_SB_VERSION_CDEV_WITH_UUID) {
			fprintf(stderr, "version verify failed.\n");
			return -1;
	}

	dump_sb(sb);
	return 0;
}

/* ./bcache_mount CACHE_DEV */
int main(int argc, char *argv[])
{
	int cache_dev_fd;
	struct cache_sb sb;
	int ret;

	if (argc != 2) {
		help_info();
		return 0;
	}

	cache_dev_fd = open(argv[1], O_RDONLY);
	if (cache_dev_fd < 0) {
		fprintf(stderr, "cache dev open failed.\n");
		return -1;
	}

	if (read_sb(cache_dev_fd, &sb))
		goto out;

	close(cache_dev_fd);
	return 0;
out:
	close(cache_dev_fd);
	return -1;
}
