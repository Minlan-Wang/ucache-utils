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
	printf("./bset_parse BUCKET CACHE_DEV\n");
}

/* ./bcache_mount CACHE_DEV */
int main(int argc, char *argv[])
{
	int ch;
	int cache_dev_fd;
	struct cache_sb sb;
	struct cache_sb_info sbi;
	int bucket = -1;
	long sector = -1;
	struct bset_info *j;
	LIST_HEAD(bset_list);
	int ret;

    while((ch = getopt(argc,argv,"b:s:"))!= EOF)
    {
        switch(ch)
        {
            case 'b':
				sscanf(optarg, "%d", &bucket);
				printf("bucket  %lu\n", bucket);
				break;
            case 's':
				sscanf(optarg, "%lu", &sector);
				printf("sector  %lu\n", sector);
				break;
            default:
				help_info();
				exit(1);
		}
    }

	argv += optind;
	argc -= optind;

	if (argc != 1) {
		help_info();
		exit(1);
	}

	cache_dev_fd = open(argv[0], O_RDONLY);
	if (cache_dev_fd < 0) {
		fprintf(stderr, "cache dev open failed.\n");
		return -1;
	}

	if (read_sb(cache_dev_fd, &sb))
		goto out;
	
	sbi.sb = &sb;
	sbi.fd = cache_dev_fd;

	if (bucket != -1)
		bucket_scan(&sbi, bucket, &bset_list);
	else if (sector != -1)
		sector_scan(&sbi, sector, &bset_list);

	dump_bset_list_bkeys(&bset_list);

	destroy_xset_list(&bset_list, struct bset_info);
	close(cache_dev_fd);
	return 0;
out:
	close(cache_dev_fd);
	return -1;
}
