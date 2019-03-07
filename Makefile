CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: server client

network.o: network.c network.h
	$(CC) $(CFLAGS) -c network.c

server.o : server.c network.h
	$(CC) $(CFLAGS) -c server.c

server: server.o network.o
	$(CC) $(CFLAGS) server.o network.o -o server $(LDFLAGS)

client.o: client.c network.h 
	$(CC) $(CFLAGS) -c client.c

client: client.o network.o
	$(CC) $(CFLAGS) client.o network.o -o client $(LDFLAGS)

clean:
	rm -f *~ *.o server client *.out *.out2

reset:
	rm -f *~ *.out *.out2 *.diff
