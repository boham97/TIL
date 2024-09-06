#!/bin/bash
all: server.out client.out

server.out: server.c
	gcc server.c -o server.out -lpthread

client.out: client.c
	gcc client.c -o client.out -lpthread


clean:
	rm -f server.out client.out
