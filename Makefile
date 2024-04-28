CC=gcc -g
CFLAGS=-Wall -Wextra

LOAD=load_balancer
SERVER=server
CACHE=lru_cache
UTILS=utils
QUEUE=queue
LINKED_LIST=linked_list
HASH_TABLE=hash_table

# Add new source file names here:
# EXTRA=<extra source file name>

.PHONY: build clean

build: tema2

tema2: main.o $(LOAD).o $(SERVER).o $(CACHE).o $(UTILS).o $(QUEUE).o $(LINKED_LIST).o $(HASH_TABLE).o
	$(CC) $^ -o $@

main.o: main.c
	$(CC) $(CFLAGS) $^ -c

$(LOAD).o: $(LOAD).c $(LOAD).h
	$(CC) $(CFLAGS) $^ -c

$(SERVER).o: $(SERVER).c $(SERVER).h
	$(CC) $(CFLAGS) $^ -c

$(CACHE).o: $(CACHE).c $(CACHE).h
	$(CC) $(CFLAGS) $^ -c

$(UTILS).o: $(UTILS).c $(UTILS).h
	$(CC) $(CFLAGS) $^ -c

# $(EXTRA).o: $(EXTRA).c $(EXTRA).h
# 	$(CC) $(CFLAGS) $^ -c
run_debug: build valgrind clean

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all ./tema2

clean:
	rm -f *.o tema2 *.h.gch

pack:
	zip distributed_db.zip *.c *.h README* Makefile 
