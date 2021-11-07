CFLAGS+= -g -O0

all: heap_sort 

heap_sort: heap_sort.o
	$(CC) -o $@ $^ 


clean:
	rm -rf *.o
	rm -rf heap_sort
