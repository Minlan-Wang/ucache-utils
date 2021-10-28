#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "bcache.h"

void dump_bkey_verbose(struct bkey *key)
{
	printf("0x%llx, 0x%llx, 0x%llx\n", key->high, key->low, key->ptr);
	printf("############################\n");
	printf("inode: %lu\n", KEY_INODE(key));
	printf("size: %lu(sectors)\n", KEY_SIZE(key));
	printf("off: %lu(sectors)\n", KEY_OFFSET(key));
	printf("dirty: %lu\n", KEY_DIRTY(key));
	printf("pinned: %lu\n", KEY_PINNED(key));
	printf("csum %lu\n", KEY_CSUM(key));
	printf("header size %lu\n", HEADER_SIZE(key));
	printf("ptrs %lu\n", KEY_PTRS(key));

	printf("\n");
	printf("ptr dev %lu\n", PTR_DEV(key, 0));
	printf("ptr offset %lu(sectors)\n", PTR_OFFSET(key, 0));
	printf("ptr gen %lu\n", PTR_GEN(key, 0));
	printf("############################\n");
}

void dump_bkey(struct bkey *key)
{
    printf("[%lu](%lu, %lu): %lu (%s)\n", KEY_INODE(key), KEY_OFFSET(key),
        KEY_SIZE(key), PTR_OFFSET(key, 0), KEY_DIRTY(key) ? "dirty": "clean");
}

/* TODO */
int merged_node_dump(struct cache_sb_info *sbi, unsigned long b)
{
	return 0;
}

int bset_bucket_dump(struct cache_sb_info *sbi, unsigned long b)
{
	struct bset *bset;
	int ret;
	int left_size;
	struct bset_info *j, *i, *k;
	struct bset_info *last;
	unsigned long last_seq;
	char *tmp;
	int bucket_size = sbi->sb->bucket_size << 9;
	int block_size = sbi->sb->block_size << 9;
	char *bucket_buf = malloc(bucket_size);
	if (!bucket_buf)
		return -1;

	memset(bucket_buf, 0, bucket_size);
	tmp = bucket_buf;
	ret = pread(sbi->fd, bucket_buf, bucket_size, b * bucket_size);
	if (ret < bucket_size) {
		fprintf(stderr, "bset bucket %u read failed.\n", b);
		goto out;
	}

	left_size = bucket_size;
	do {
		bset = (struct bset *)bucket_buf;

		/*
		 * keep magic check first. otherwisze csum_set() may get a invalid len
		 * which causes segment false.
		 */
		if (bset->magic != bset_magic(sbi->sb)) {
			printf("bset magic check failed.\n");
			goto out;
		}

		if (bset->version != BCACHE_BSET_VERSION) {
			printf("bucket %d, bad version 0x%lx\n",
					b, bset->version);
			goto out;
		}

		if (bset->csum != btree_csum_set(&sbi->root, bset)) {
			printf("bucket %d csum 0x%lx, expect csum 0x%lx\n",
					b, csum_set(bset), bset->csum);
			goto out;
		}

		if (left_size < set_size(bset))
			break;

		j = malloc(set_size(bset) + offsetof(struct bset_info, bset));
		if (!j) {
			fprintf(stderr, "Out of memory.\n");
			break;
		}
		memcpy(j->bset, bset, set_size(bset));

		printf("bucket %d, offset %lu, bset size %d, left size %d, seq %lu\n",
			b, bucket_size - left_size, set_size(bset), left_size, j->bset->seq);

		dump_xset_bkeys(bset);

		left_size -= set_size_aligned(bset, block_size);
		bucket_buf += set_size_aligned(bset, block_size);

	} while(left_size);

	free(tmp);
	return 0;
out:
	free(tmp);
	return -1;
}