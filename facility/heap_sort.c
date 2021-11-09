#include <stdio.h>
#include <stdlib.h>

/* 实现max heap */

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

struct heap_t {
	int size, used;
	unsigned long *data;
};

typedef int (cmp_t)(unsigned long a, unsigned long b);

int foo_cmp(unsigned long a, unsigned long b)
{
	return a < b;
}

unsigned long* init_heap(struct heap_t *heap, int _size)
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
	unsigned long t;

	t = h->data[i];
	h->data[i] = h->data[j];
	h->data[j] = t;
}

/* 往叶子节点方向swap */
void heap_sift(struct heap_t *h, int i, cmp_t *cmp)
{
	int _r, _j = i;

	for (; _j * 2 + 1 < h->used; _j = _r) {
		/* _j的left child */
		_r = _j * 2 + 1;
		/* _j的right child */
		if (_r + 1 < h->used &&
			/* 左右节点比较,如果left child < right child,那么就选择right child */
		    cmp(h->data[_r], h->data[_r + 1]))
			_r++;

		/* 较大的那个child如果小于_j,表示符合max heap规则,可以退出*/
		if (cmp(h->data[_r], h->data[_j]))
			break;
		/* 否则_j和大的那个child交换 */
		heap_swap(h, _r, _j);
	}
}

/* 从i往root方向swap */
void heap_sift_down(struct heap_t *h, int i, cmp_t *cmp)
{
	int p;

	while (i) {
		/* 找到父节点 */
		p = (i - 1) / 2;
		/* 如果当前节点小于父节点, 就退出*/
		if (cmp(h->data[i], h->data[p]))
			break;
		/* 如果当前节点大于父节点,交换父节点和当前节点 */
		heap_swap(h, i, p);
		i = p;
	}

	return;
}

int heap_add(struct heap_t *h, int d, cmp_t *cmp)
{
	int _r = !heap_full(h);
	if (_r) {
		/* 为了保证complete binary tree的结构, 每次添加元素到最后 */
		int _i = h->used++;
		(h)->data[_i] = d;

		heap_sift_down(h, _i, cmp);
		//heap_sift(h, _i, cmp);
	}

	return _r;
}

int heap_pop(struct heap_t *h, unsigned long *d, cmp_t *cmp)
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
	unsigned long v;
	struct heap_t heap;
	unsigned long array[TEST_NUM] = {10, 20, 15, 12, 40, 25, 18, 90, 70, 80};

	init_heap(&heap, TEST_NUM);

	for(i = 0; i < TEST_NUM; i++) {
		heap_add(&heap, array[i], foo_cmp);
	}

	printf("print heap ############\n");
	for (i = 0; i < TEST_NUM; i++) {
		printf("%lu\n", heap.data[i]);
	}

	printf("heap sort #############\n");
	for (i = 0; i < TEST_NUM; i++) {
		heap_pop(&heap, &v, foo_cmp);
		printf("%lu\n", v);
	}
    return 0;
}

