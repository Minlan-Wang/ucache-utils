#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "bcache_fs.h"
#include "bcache.h"
/*
 * size_t: unsigned long
 * off_t: unsigned long
 * ssize_t: unsigned int
 */

static void help_info(void)
{
	printf("./jset_parse BUCKET CACHE_DEV\n");
}

/* ./bcache_mount CACHE_DEV */
int main(int argc, char *argv[])
{
	int cache_dev_fd;
	struct cache_sb sb;
	struct cache_sb_info sbi;
	int bucket;
	struct jset_info *j;
	LIST_HEAD(replay_list);
	int ret;

	if (argc != 3) {
		help_info();
		return 0;
	}

	cache_dev_fd = open(argv[2], O_RDONLY);
	if (cache_dev_fd < 0) {
		fprintf(stderr, "cache dev open failed.\n");
		return -1;
	}

	sscanf(argv[1], "%d", &bucket);

	if (read_sb(cache_dev_fd, &sb))
		goto out;
	
	sbi.sb = &sb;
	sbi.fd = cache_dev_fd;

	journal_bucket_dump(&sbi, bucket);
	close(cache_dev_fd);
	return 0;
out:
	close(cache_dev_fd);
	return -1;
}
