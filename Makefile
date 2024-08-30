PROJECT=server
SOURCES=
LIBRARY=nope
INCPATHS=include
LIBPATHS=.
LDFLAGS=-std=c++1z -g
CFLAGS=-c -Wall -g
CC=g++
IP_SERVER = 127.0.0.1
PORT = 12345
# Automatic generation of some important lists
OBJECTS=$(SOURCES:.c=.o)
INCFLAGS=$(foreach TMP,$(INCPATHS),-I$(TMP))
LIBFLAGS=$(foreach TMP,$(LIBPATHS),-L$(TMP))

# Set up the output file names for the different output types
BINARY=$(PROJECT)

all: server subscriber

server:
	$(CC) $(OBJECTS) server.cpp $(LDFLAGS) -o server

subscriber:
	$(CC) $(OBJECTS) subscriber.cpp $(LDFLAGS) -o subscriber

distclean: clean
	rm -f $(BINARY)

run_server: server
	./server ${PORT}

clean:
	rm -f $(OBJECTS) server.o server subscriber.o subscriber

