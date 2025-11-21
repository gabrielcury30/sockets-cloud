CC = gcc

CFLAGS = -Wall -g

BINS = server client

all: $(BINS)

server: server.c 
		$(CC) $(CFLAGS) -o server server.c
client: client.c 
		$(CC) $(CFLAGS) -o client client.c

clean: 
		rm -f $(BINS)