CC = gcc
CFLAGS = -Wall -pedantic -std=gnu99
LDFLAGS = -pthread

all: server client
server : server.o  utils.o
client : client.o

clean: rm -f server *.o

.PHONY: all clean
