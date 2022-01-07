all: client server

clear:
	rm -f *.o

client: client.c
	gcc -o client client.c -lncurses -lpthread

server: server.c
	gcc -o server server.c -lncurses -lpthread