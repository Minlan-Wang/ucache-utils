#include <stdio.h>
#include <stdlib.h>

/* 实现min heap */
#define heap_full(h)	((h)->used == (h)->size)
#if 0
#define DECLARE_HEAP(type, name)					\
	struct {							\
		size_t size, used;					\
		type *data;						\
	} name

#define init_heap(heap, _size, gfp)					\
({									\
	size_t _bytes;							\
	(heap)->used = 0;						\
	(heap)->size = (_size);						\
	_bytes = (heap)->size * sizeof(*(heap)->data);			\
	(heap)->data = kvmalloc(_bytes, (gfp) & GFP_KERNEL);		\
	(heap)->data;							\
})

#define free_heap(heap)							\
do {									\
	kvfree((heap)->data);						\
	(heap)->data = NULL;						\
} while (0)

#define heap_swap(h, i, j)	swap((h)->data[i], (h)->data[j])

#define heap_sift(h, i, cmp)						\
do {									\
	size_t _r, _j = i;						\
									\
	for (; _j * 2 + 1 < (h)->used; _j = _r) {			\
		_r = _j * 2 + 1;					\
		if (_r + 1 < (h)->used &&				\
		    cmp((h)->data[_r], (h)->data[_r + 1]))		\
			_r++;						\
									\
		if (cmp((h)->data[_r], (h)->data[_j]))			\
			break;						\
		heap_swap(h, _r, _j);					\
	}								\
} while (0)

#define heap_sift_down(h, i, cmp)					\
do {									\
	while (i) {							\
		size_t p = (i - 1) / 2;					\
		if (cmp((h)->data[i], (h)->data[p]))			\
			break;						\
		heap_swap(h, i, p);					\
		i = p;							\
	}								\
} while (0)

#define heap_add(h, d, cmp)						\
({									\
	bool _r = !heap_full(h);					\
	if (_r) {							\
		size_t _i = (h)->used++;				\
		(h)->data[_i] = d;					\
									\
		heap_sift_down(h, _i, cmp);				\
		heap_sift(h, _i, cmp);					\
	}								\
	_r;								\
})

#define heap_pop(h, d, cmp)						\
({									\
	bool _r = (h)->used;						\
	if (_r) {							\
		(d) = (h)->data[0];					\
		(h)->used--;						\
		heap_swap(h, 0, (h)->used);				\
		heap_sift(h, 0, cmp);					\
	}								\
	_r;								\
})

#define heap_peek(h)	((h)->used ? (h)->data[0] : NULL)

#else
/*
 * 下标和父子关系
 * A B C D E F G
 * 0 1 2 3 4 5 6
 * 一个节点当前index为x(x > 0), 的父节点index为(x - 1)/2.
 * 一个节点当前index为x,子节点的index为(2x+1), 2x+2
*/

/* 以key排序(最小堆),key可能相同 */
struct kv {
	unsigned long key;
	unsigned long value;
};

struct heap_t {
	int size, used;
	struct kv *data;
};

typedef long (cmp_t)(struct kv a, struct kv b);

/* 最小堆 */
long foo_cmp(struct kv a, struct kv b)
{
	return ((long)a.key - (long)b.key) > 0;
}

struct kv* init_heap(struct heap_t *heap, int _size)
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
	struct kv t;

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

int heap_add(struct heap_t *h, struct kv d, cmp_t *cmp)
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

int heap_pop(struct heap_t *h, struct kv *d, cmp_t *cmp)
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
#endif

#define TEST_NUM 10	
int main()
{
	int i;
	struct kv v;
	struct heap_t heap;
	struct kv array[TEST_NUM] = {{10, 1}, {15, 1}, {12, 1},
					{40, 1}, {15, 2}, {18, 1}, {90, 1},
					{70, 1}, {15, 3}, {10, 2}};
#if 0
    /* real situation */
	struct kv array[TEST_NUM] = {{87920, 2850816}, {0, 2890752}, {0, 2868224},
					{0, 1294336}, {0, 801792}};
#endif

	init_heap(&heap, TEST_NUM);

	for(i = 0; i < TEST_NUM; i++) {
		heap_add(&heap, array[i], foo_cmp);
	}

	printf("print heap ############\n");
	for (i = 0; i < TEST_NUM; i++) {
		printf("%lu %lu\n", heap.data[i].key, heap.data[i].value);
	}

	printf("heap sort #############\n");
	for (i = 0; i < TEST_NUM; i++) {
		heap_pop(&heap, &v, foo_cmp);
		printf("%lu %lu\n", v.key, v.value);
	}
    return 0;
}

