CFLAGS = -Wall -lm -Wno-incompatible-pointer-types
ID = C1
PORT = 8313
IP_SERVER = 127.0.0.1

all: server subscriber

server: server.c functions.c

subscriber: subscriber.c functions.c

.PHONY: clean run_server run_subscriber

run_server:
	./server ${PORT}

run_subscriber:
	./subscriber $(ID) ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
