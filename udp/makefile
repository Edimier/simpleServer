.PHONY: all  clean

all : main client broadcast_client broadcast_server

main:
	gcc -o main main.c -lpthread
client:
	gcc -o client client.c -DCONNECT

broadcast_client:
	gcc -o broadcast_client broadcast_client.c

broadcast_server:
	gcc -o broadcast_server broadcast_server.c

clean :
	rm -f main client broadcast_server broadcast_client
