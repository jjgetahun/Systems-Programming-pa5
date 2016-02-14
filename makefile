all: net

net:
	gcc -pthread -o client myclient.c
	gcc -pthread -o server myserver.c -lm

clean:
	rm -rf *.o client server
