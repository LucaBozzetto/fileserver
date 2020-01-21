CC = gcc

CFLAGS = -Wall
REQUIRED = networkStuff.h networkHelpers.c 

all: server client

debug: CFLAGS += -DDEBUG -g
debug: all

#create server binary
server: server.c $(REQUIRED) queueCondition.h queueCondition.c
	$(CC) $(CFLAGS) server.c networkHelpers.c queueCondition.c -o server -lm -lpthread;

#create client binary
client: client.c $(REQUIRED)
	$(CC) $(CFLAGS) client.c networkHelpers.c -o client -lm -lpthread;

# removes all the binaries
clean: 
	$(RM) ./server ./client
