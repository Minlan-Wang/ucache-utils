CFLAGS+= -g -O0

all: bkey_parse bset_parse jset_parse bcache_super \
	bcache_mount journal_dump root_dump

bcache_super: bcache_super.o
	$(CC) -o $@ $^ -luuid

bcache_mount: bcache_mount.o journal.o super.o btree.o
	$(CC) -o $@ $^ -luuid

root_dump: root_dump.o journal.o super.o btree.o
	$(CC) -o $@ $^ -luuid

node_dump: node_dump.o journal.o super.o btree.o
	$(CC) -o $@ $^ -luuid

journal_dump: journal_dump.o journal.o super.o btree.o
	$(CC) -o $@ $^ -luuid

jset_parse: jset_parse.o journal.o super.o btree.o
	$(CC) -o $@ $^ -luuid

bkey_parse: bkey_parse.o btree.o
	$(CC) -o $@ $^

bset_parse: bset_parse.o journal.o super.o btree.o
	$(CC) -o $@ $^ -luuid

clean:
	rm -rf *.o
	rm -rf bkey_parse bset_parse bcache_super bcache_mount\
		jset_parse journal_dump root_dump node_dump
