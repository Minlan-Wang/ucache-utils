#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include "bcache_fs.h"
#include "bcache.h"

/* Don't kick out staled jset entry */
int journal_bucket_dump(struct cache_sb_info *sbi, int b)
{
	struct jset *jset;
	int ret;
	int left_size;
	struct jset_info *j, *i, *k;
	char *tmp;
	int bucket_size = sbi->sb->bucket_size << 9;
	int block_size = sbi->sb->block_size << 9;
	char *bucket_buf = malloc(bucket_size);
	if (!bucket_buf)
		return -1;

	tmp = bucket_buf;
	ret = pread(sbi->fd, bucket_buf, bucket_size, b * bucket_size);
	if (ret < bucket_size) {
		fprintf(stderr, "journal bucket %u read failed.\n", b);
		goto out;
	}

	left_size = bucket_size;
	do {
		jset = (struct jset *)bucket_buf;

		/*
		 * keep magic check first. otherwisze csum_set() may get a invalid len
		 * which causes segment false.
		 */
		if (jset->magic != jset_magic(sbi->sb)) {
			goto out;
		}

		if (jset->csum != csum_set(jset)) {
			printf("journal bucket %d csum 0x%lx, expect csum 0x%lx\n",
					b, csum_set(jset), jset->csum);
			goto out;
		}

		if (jset->version != BCACHE_JSET_VERSION) {
			printf("journal bucket %d, bad version 0x%lx\n",
					b, jset->version);
			goto out;
		}

		if (set_size_aligned(jset, block_size) > (PAGE_SECTORS << JSET_BITS) << 9) {
			printf("jset too big (%lu), skip\n", set_size(jset));
			goto out;
		}

		if (left_size < set_size(jset))
			break;

		printf("bucket %d, offset %lu, jset size %d, left size %d, seq %lu, last_seq %lu\n",
			b, bucket_size - left_size, set_size(jset), left_size, jset->seq, jset->last_seq);

		dump_xset_bkeys(jset);
		left_size -= set_size_aligned(jset, block_size);
		bucket_buf += set_size_aligned(jset, block_size);

	} while(left_size);

	free(tmp);
	return 0;
out:
	free(tmp);
	return -1;
}

int journal_bucket_read(struct cache_sb_info *sbi, int b, struct list_head *head)
{
	struct jset *jset;
	int ret;
	int left_size;
	struct jset_info *j, *i, *k;
	struct list_head *where;
	char *tmp;
	int bucket_size = sbi->sb->bucket_size << 9;
	int block_size = sbi->sb->block_size << 9;
	char *bucket_buf = malloc(bucket_size);
	if (!bucket_buf)
		return -1;

	tmp = bucket_buf;
	ret = pread(sbi->fd, bucket_buf, bucket_size, b * bucket_size);
	if (ret < bucket_size) {
		fprintf(stderr, "journal bucket %u read failed.\n", b);
		goto out;
	}

	left_size = bucket_size;
	do {
restart:
		jset = (struct jset *)bucket_buf;

		/*
		 * keep magic check first. otherwisze csum_set() may get a invalid len
		 * which causes segment false.
		 */
		if (jset->magic != jset_magic(sbi->sb)) {
		//	printf("journal bucket %d magic 0x%lx, expect magic 0x%lx\n",
		//			b, jset->magic, jset_magic(sbi->sb));
			goto out;
		}

		if (jset->csum != csum_set(jset)) {
			printf("journal bucket %d csum 0x%lx, expect csum 0x%lx\n",
					b, csum_set(jset), jset->csum);
			goto out;
		}

		if (jset->version != BCACHE_JSET_VERSION) {
			printf("journal bucket %d, bad version 0x%lx\n",
					b, jset->version);
			goto out;
		}

		if (set_size_aligned(jset, block_size) > (PAGE_SECTORS << JSET_BITS) << 9) {
			printf("jset too big (%lu), skip\n", set_size(jset));
			goto out;
		}

		if (left_size < set_size(jset))
			break;


		j = malloc(set_size(jset) + offsetof(struct jset_info, jset));
		if (!j) {
			fprintf(stderr, "Out of memory.\n");
			break;
		}
		memcpy(j->jset, jset, set_size(jset));

		printf("bucket %d, offset %lu, jset size %d, left size %d, seq %lu, last_seq %lu\n",
			b, bucket_size - left_size, set_size(jset), left_size, j->jset->seq, j->jset->last_seq);

		left_size -= set_size_aligned(jset, block_size);
		bucket_buf += set_size_aligned(jset, block_size);

		/* filter out staled entry */
		list_for_each_entry_safe(i, k, head, list) {
			if (i->jset->seq >= j->jset->last_seq)
				break;

			printf("j seq %lu, last_seq %lu, kick i seq 0x%lu, last_seq %lu\n",
			j->jset->seq, j->jset->last_seq, i->jset->seq, i->jset->last_seq);
			list_del(&i->list);
			free(i);
		}

		list_for_each_entry_reverse(i, head, list) {
			assert(i->jset->seq != j->jset->seq);

			if (j->jset->seq < i->jset->last_seq)
				goto restart;
			if (j->jset->seq > i->jset->seq) {
				where = &i->list;
				goto add;
			}
		}
		where = head;
add:
		list_add(&j->list, where);
	} while(left_size);

	free(tmp);
	return 0;
out:
	free(tmp);
	return -1;
}

void dump_replay_list(struct list_head *head, bool v)
{
	struct jset_info *j;

	printf("dumping replay list.........\n");
	list_for_each_entry(j, head, list) {
		printf("jset seq %lu, last seq %lu, keys %u\n", j->jset->seq,
			j->jset->last_seq, j->jset->keys);

		if(v)
			dump_xset_bkeys(j->jset);
	}
}

int journal_read(struct cache_sb_info *sbi, struct list_head *head)
{
	int i;
	int ret = 0;

	for (i = 0; i < sbi->sb->njournal_buckets; i++) {
		journal_bucket_read(sbi, i, head);
	}

	return 0;
}