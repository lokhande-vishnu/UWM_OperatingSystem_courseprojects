#####################################################################
#
# Type "make" or "make myprogram" to compile your code
#
# Type "make clean" to remove the executable (and any object files)
#
#####################################################################

CC=gcc
CFLAGS=-Wall -g -Werror

all: stats_server libstats.so stats_client

stats_server: stats_server.c
	$(CC) -o stats_server $(CFLAGS) stats_server.c -lpthread

libstats.so: stats.c
	$(CC) -c -fpic stats.c $(CFLAGS) -lpthread
	gcc -shared -o libstats.so stats.o

stats_client:
	$(CC) -c -fpic stats.c $(CFLAGS) -lpthread
	gcc -shared -o libstats.so stats.o
	$(CC) -o stats_client stats_client.c $(CFLAGS) -L$(shell pwd) -lstats -lpthread

clean:
	$(RM) stats_server stats_client libstats.so stats.o
