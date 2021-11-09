#include <stdio.h>
#include <stdlib.h>

/* 实现min heap */
#define heap_full(h)	((h)->used == (h)->size)
/*
 * 下标和父子关系
 * A B C D E F G
 * 0 1 2 3 4 5 6
 * 一个节点当前index为x(x > 0), 的父节点index为(x - 1)/2.
 * 一个节点当前index为x,子节点的index为(2x+1), 2x+2
*/

/* 以key排序(最小堆),key可能相同 */
struct kv {
	/* bkey simulation */
	unsigned long key;
	/* bkey ptr simulation */
	unsigned long value;
};

struct heap_t {
	int size, used;
	struct kv **data;
};

typedef long (cmp_t)(struct kv *a, struct kv *b);

/* 不排序,按照fifo加入 */
long no_cmp(struct kv *a, struct kv *b)
{
	return 1;
}

/* 最小堆 */
long foo_cmp(struct kv *a, struct kv *b)
{
	return ((long)a->key - (long)b->key) > 0;
}

/* 在key相等的情况下,需要以地址排序 */
long btree_cmp(struct kv *a, struct kv *b)
{
	long c = foo_cmp(a, b);
	return c ? c > 0 : a < b;
}

struct kv** init_heap(struct heap_t *heap, int _size)
{
	int _bytes;

	heap->used = 0;
	heap->size = _size;
	_bytes = heap->size * sizeof(*heap->data);
	heap->data = malloc(_bytes);

	return heap->data;
}

void free_heap(struct heap_t *heap)
{
	free(heap->data);
	heap->data = NULL;
}

void heap_swap(struct heap_t *h, int i, int j)
{
	struct kv *t;

	t = h->data[i];
	h->data[i] = h->data[j];
	h->data[j] = t;
}

/* 往叶子节点方向swap */
void heap_sift(struct heap_t *h, int i, cmp_t *cmp)
{
	int _r, _j = i;

	/* _j * 2 + 1是左节点的index */
	for (; _j * 2 + 1 < h->used; _j = _r) {
		/* _j的left child */
		_r = _j * 2 + 1;
		/* _j的right child */
		if (_r + 1 < h->used &&
			/* 左右节点比较,如果left child < right child,那么就选择right child */
		    cmp(h->data[_r], h->data[_r + 1]))
			_r++;

		/* 较大的那个child如果大于_j,表示符合min heap规则,可以退出*/
		if (cmp(h->data[_r], h->data[_j]))
			break;
		/* 否则_j和大的那个child交换 */
		heap_swap(h, _r, _j);
	}
}

/* 从i往root方向swap */
int heap_sift_down(struct heap_t *h, int i, cmp_t *cmp)
{
	int p;

	while (i) {
		/* 找到父节点 */
		p = (i - 1) / 2;
		/* 如果当前节点大于于父节点, 就退出*/
		if (cmp(h->data[i], h->data[p]))
			break;
		/* 如果当前节点大于父节点,交换父节点和当前节点 */
		heap_swap(h, i, p);
		i = p;
	}

	return i;
}

int heap_add(struct heap_t *h, struct kv *d, cmp_t *cmp)
{
	int _r = !heap_full(h);
	if (_r) {
		/* 为了保证complete binary tree的结构, 每次添加元素到最后 */
		int _i = h->used++;
		(h)->data[_i] = d;

		_i = heap_sift_down(h, _i, cmp);
		/* 为了处理相同key的情况. */
		heap_sift(h, _i, cmp);
	}

	return _r;
}

int heap_pop(struct heap_t *h, struct kv **d, cmp_t *cmp)
{
	int _r = h->used;

	if (_r) {
		/* 每次取出root */
		*d = h->data[0];
		h->used--;
		/* heap最后一个元素和root交换 */
		heap_swap(h, 0, h->used);
		/* root向下swap */
		heap_sift(h, 0, cmp);
	}

	return _r;
}

#define heap_peek(h)	((h)->used ? (h)->data[0] : NULL)

#define TEST_NUM 13
int main()
{
	int i;
	struct kv *v;
	struct heap_t heap;

    /* real situation */
	struct kv array[TEST_NUM] = {{87920, 1883136}, {0, 4514816}, {0, 1130496},
					{0, 1884160}, {0, 4324352}, {0, 4320256}, {0, 1857536},
					{0, 1870848}, {0, 1102848}, {0, 1883136}, {0, 4326400},
					{0, 4316160}, {0, 4321280}};

	init_heap(&heap, TEST_NUM);

	for(i = 0; i < TEST_NUM; i++) {
		/* 即便不排序,插入heap,对heap pop也毫无影响.因为之后heapify会堆化 */
		heap_add(&heap, &array[i], no_cmp);
#if 0
		/* 按照foo_cmp进行排序 */
		heap_add(&heap, &array[i], foo_cmp);
#endif
	}

	printf("print heap ############\n");
	for (i = 0; i < TEST_NUM; i++) {
		printf("%lu %lu\n", heap.data[i]->key, heap.data[i]->value);
	}

	printf("print heapify ############\n");
	for (i = heap.used / 2 - 1; i >= 0; --i)
		heap_sift(&heap, i, btree_cmp);

	for (i = 0; i < TEST_NUM; i++) {
		printf("%lu %lu\n", heap.data[i]->key, heap.data[i]->value);
	}

	printf("heap sort #############\n");
	for (i = 0; i < TEST_NUM; i++) {
		heap_pop(&heap, &v, btree_cmp);
		printf("%lu %lu\n", v->key, v->value);
	}
    return 0;
}

