.PHONY: all  clean

all : main client

main:
	gcc -o main main.c -lpthread
client:
	gcc -o client client.c -DCONNECT

clean :
	rm -f main client
