CFLAGS+= -g -O0

all: heap_sort heap_sort_bset

heap_sort_bset: heap_sort_bset.o
	$(CC) -o $@ $^ 

heap_sort: heap_sort.o
	$(CC) -o $@ $^ 

clean:
	rm -rf *.o
	rm -rf heap_sort
	rm -rf heap_sort_bset
