#include <stdio.h>
#include <assert.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define DECLARE_ARRAY_ALLOCATOR(type, name, size)			\
    struct {    \
		type	*freelist;					\
		type	data[size];					\
	} name

struct XXX {	
    unsigned long	*freelist;	
    unsigned long	data[500];
};

struct test {
#if 0
	DECLARE_ARRAY_ALLOCATOR(int, freelist, 500);
#else
    struct XXX freelist;
#endif
};

#if 0

#define array_alloc(array)						\
({									\
	typeof((array)->freelist) _ret = (array)->freelist;		\
									\
	if (_ret)							\
		(array)->freelist = *((typeof((array)->freelist) *) _ret);\
									\
	_ret;								\
})

#define array_free(array, ptr)						\
do {									\
	typeof((array)->freelist) _ptr = ptr;				\
									\
	*((typeof((array)->freelist) *) _ptr) = (array)->freelist;	\
	(array)->freelist = _ptr;					\
} while (0)

//assert(sizeof((array)->data[0]) >= sizeof(void *));
#define array_allocator_init(array)					\
do {									\
	typeof((array)->freelist) _i;					\
									\
	(array)->freelist = NULL;					\
									\
	for (_i = (array)->data;					\
	     _i < (array)->data + ARRAY_SIZE((array)->data);		\
	     _i++)							\
		array_free(array, _i);					\
} while (0)
#else
unsigned long *array_alloc(struct XXX *array)
{
	unsigned long *_ret = (array)->freelist;

	if (_ret)
        /*
         * 下面语句等同于
         * (array)->freelist = (unsigned long *)(*_ret);
         *
         * 其实就是指向下一个obj.下面引入了二级指针,看上去复杂.但是实际
         * 上二级指针对计算机来说只是一个地址,和一级指针没什么不同.
         * 结果也就是对指针解引用,最终获取到一级指针.
         */
		(array)->freelist = *((unsigned long **) _ret);

	return _ret;
}

/* 将ptr归还freelist */
void array_free(struct XXX *array, unsigned long *ptr)
{
	unsigned long *_ptr = ptr;

	*((unsigned long **) _ptr) = array->freelist;
	(array)->freelist = _ptr;
}

void  array_allocator_init(struct XXX *array)
{
	unsigned long * _i;

	(array)->freelist = NULL;

	for (_i = (array)->data;
	     _i < (array)->data + ARRAY_SIZE((array)->data);
	     _i++)
		array_free(array, _i);
}
#endif


int main()
{
    int i;
	struct test tt;
    unsigned long *p;
	array_allocator_init(&tt.freelist);

    for (i = 0; i < 10; i++) {
        p = array_alloc(&tt.freelist);
        printf("0x%lx\n", (unsigned long)p);
    }
    return 0;
}

