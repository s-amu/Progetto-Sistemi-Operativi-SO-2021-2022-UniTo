export CC = gcc
export CFLAGS = -std=c89 -pedantic -Wall -Werror

all: master users nodes

master: master.o master_functions.o
	$(CC) $(CFLAGS) -o master master.o master_functions.o

users: users.o users_functions.o
	$(CC) $(CFLAGS) -o users users.o users_functions.o

nodes: nodes.o nodes_functions.o
	$(CC) $(CFLAGS) -o nodes nodes.o nodes_functions.o

master.o: master.c master.h header.h
	$(CC) $(CFLAGS) -c master.c

master_functions.o: master_functions.c
	$(CC) $(CFLAGS) -c master_functions.c

users.o: users.c users.h header.h
	$(CC) $(CFLAGS) -c users.c

users_functions.o: users_functions.c
	$(CC) $(CFLAGS) -c users_functions.c

nodes.o: nodes.c nodes.h header.h
	$(CC) $(CFLAGS) -c nodes.c

nodes_functions.o: nodes_functions.c
	$(CC) $(CFLAGS) -c nodes_functions.c

run:
	./master "macro.txt"

clean:
	rm -f *.o
	rm master users nodes