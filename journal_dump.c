#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "bcache.h"
//#include <assert.h>

/*
 * size_t: unsigned long
 * off_t: unsigned long
 * ssize_t: unsigned int
 */
static void help_info(void)
{
	printf("./journal_dump [-k] CACHE_DEV\n");
}

int main(int argc, char *argv[])
{
	int cache_dev_fd;
	struct cache_sb sb;
	struct cache_sb_info sbi;
	struct jset_info *j;
	int dump_bkey = 0;
	LIST_HEAD(replay_list);
	int ret, ch;

    while((ch = getopt(argc, argv,"k"))!= EOF)
    {
        switch(ch)
        {
            case 'k':
				dump_bkey = 1;
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

	journal_read(&sbi, &replay_list);
	dump_replay_list(&replay_list, dump_bkey);

	destroy_xset_list(&replay_list, struct jset_info);
	close(cache_dev_fd);
	return 0;
out:
	close(cache_dev_fd);
	return -1;
}
