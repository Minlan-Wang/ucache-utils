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

void dump_bset_list_bkeys(struct list_head *head)
{
	struct bset_info *j;
	struct bkey *k;

	printf("dumping bset list.........\n");
	list_for_each_entry(j, head, list) {
		printf("jset seq %lu, keys %u\n", j->bset->seq, j->bset->keys);
		dump_xset_bkeys(j->bset);
	}
}

int bucket_scan(struct cache_sb_info *sbi, int b, struct list_head *head)
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
			goto out;
		}

		if (bset->csum != csum_set(bset)) {
			printf("bucket %d csum 0x%lx, expect csum 0x%lx\n",
					b, csum_set(bset), bset->csum);
			goto out;
		}

		if (bset->version != BCACHE_BSET_VERSION) {
			printf("bucket %d, bad version 0x%lx\n",
					b, bset->version);
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

		left_size -= set_size_aligned(bset, block_size);
		bucket_buf += set_size_aligned(bset, block_size);

		list_add_tail(&j->list, head);

	} while(left_size);

	free(tmp);
	return 0;
out:
	free(tmp);
	return -1;
}

int sector_scan(struct cache_sb_info *sbi, long sector, struct list_head *head)
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

	tmp = bucket_buf;
	ret = pread(sbi->fd, bucket_buf, bucket_size, sector << 9);
	if (ret < bucket_size) {
		fprintf(stderr, "bset sector %ld read failed.\n", sector);
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
			goto out;
		}

		if (bset->csum != csum_set(bset)) {
			printf("sector %d csum 0x%lx, expect csum 0x%lx\n",
					sector, csum_set(bset), bset->csum);
			goto out;
		}

		if (bset->version != BCACHE_BSET_VERSION) {
			printf("sector %d, bad version 0x%lx\n",
					sector, bset->version);
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

		printf("sector %d, offset %lu, bset size %d, left size %d, seq %lu\n",
			sector, bucket_size - left_size, set_size(bset), left_size, j->bset->seq);

		left_size -= set_size_aligned(bset, block_size);
		bucket_buf += set_size_aligned(bset, block_size);

		list_add_tail(&j->list, head);

	} while(left_size);

	free(tmp);
	return 0;
out:
	free(tmp);
	return -1;
}