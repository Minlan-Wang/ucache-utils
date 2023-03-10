#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "bcache.h"
#include  <string.h>
//#include <assert.h>

/*
 * size_t: unsigned long
 * off_t: unsigned long
 * ssize_t: unsigned int
 */
static void help_info(void)
{
	printf("./root_dump CACHE_DEV\n");
}

/* ./root_dump CACHE_DEV */
int main(int argc, char *argv[])
{
	int cache_dev_fd;
	struct cache_sb sb;
	struct cache_sb_info sbi;
	struct jset_info *j;
	LIST_HEAD(replay_list);
	int ret, ch;
	struct bkey *k;
	unsigned long b;
	unsigned int node_blocks;
	int merged;

    while((ch = getopt(argc,argv,"m"))!= EOF)
    {
        switch(ch)
        {
            case 'm':
				merged = 1;
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

	node_blocks = sb.bucket_size / sb.block_size;

	if (sb.bucket_size > BTREE_MAX_SECTORS)
		 sbi.node_blocks = max_t(unsigned int, node_blocks / 4,
			BTREE_MAX_SECTORS / sb.block_size);

	journal_read(&sbi, &replay_list);
	dump_replay_list(&replay_list, false);

	j = list_last_entry(&replay_list, struct jset_info, list);
	bkey_copy(&sbi.root.key, &j->jset->btree_root);
	sbi.root.btree_level = j->jset->btree_level;

	b = sector_to_bucket(&sbi, PTR_OFFSET(&sbi.root.key, 0));
	printf("root node bucket %d, blocks %u, level %u\n", b, sbi.node_blocks,
		sbi.root.btree_level);

	if (merged)
		merged_node_dump(&sbi, b);
	else
		btree_node_dump(&sbi, PTR_OFFSET(&sbi.root.key, 0));

	destroy_xset_list(&replay_list, struct jset_info);
	close(cache_dev_fd);
	return 0;
out:
	close(cache_dev_fd);
	return -1;
}
