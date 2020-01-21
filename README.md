# Upload server in C
A really simple multi-threaded file server developed in C.

![screenshot](https://user-images.githubusercontent.com/2963072/65913230-cf781680-e3cf-11e9-8327-8bd6c9d61072.png)

This project was made for the [Computer Networks](http://www.dsi.unive.it/~bergamasco/teaching.html) course @[University of Guelph](https://www.uoguelph.ca/).\


## Installation and Usage
Running `make` will create both client and server executable. If you want to create only one of them you can simply type `make server` or `make client`.\
The `clean` command simply deletes both the client and server binaries.\
Please mind if you want the client executable to sleep a few seconds after a certain percentage of file is sent, run the command `make debug`.\


The command line arguments for the server are:
	- `-a listenAddress` = set the address where the server will be listening on (the port is the one I was given and cannot be changed). If the address is not passed the server will listen on localhost
For the client the arguments are:
	- `-a address` = the address where the client will try to send the file, if not provided localhost will be used instead
	- `-f` = the file path of the file to be sent. This argument is always required.
	- `-c` = the chunk size of the various messages sent by the client while transferring the file. If not provided the maximum value set(CHUNK_SIZE) will be used instead. Please mind if the server has a lower maximum chunk size client and server will renegotiate the maximum chunk size so a lower value for the chunk size may be used instead.

I've provided a little bash script to run multiple clients (dos.sh :D). Run it with ./dos.sh $1 $2 $3 where:
 - `$1` = number of clients to launch in parallel
 - `$2` = address where the clients will connect
 - `$3` = file path of the file to be sent
You can skip `$2` and all the clients will connect to localhost instead.

## Features
The server uses TCP sockets and its job is to receive files from various clients and save the files in the `uploads` folder.\
The server is threaded. The main thread listens for connections. When a new client connects, the main thread starts a “transfer” thread and passes the client address info to the thread function.\
A transfer thread will receive a file (with a file name) from the client, and will save it to disk on the server side. Once the file has been received and saved, the transfer thread exits.\
The server will run a simple command-line user menu in a separate UI thread. It will provide the user with the following options:\
 - Display active transfers: This shows the names of the files being downloaded.
 - Shut down the server: tells the main thread to terminate the program.

If the user selects the shut down option, I offer 2 choices:
 - Hard shut down: the UI server tells the main server to exit, the main server terminates immediately.
 - Soft shut down: the server stops accepting new connections and checks if there are any active connections
   - If there are none,the server exits.
   - If connections are in progress, the server asks the user if they should be terminated or if the user wants for them to be completed. If the user wants to exit immediately, exit the server. Otherwise, the main thread will wait for the transfers to be completed using pthread_join before exiting.

THe server prevents file collisions. I've used the function `mkstemps()` which is thread safe.\
Given a file name containing the char 'X' one or multiple times it will substitute the 'X' chars with a generated sequence.\
Since I've inserted 6 X chars there's space for a lot of file uploads with the same name.\
For more information about the mkstemps function visit [this link](https://linux.die.net/man/3/mkstemp).\


## protocol
I have imposed a simple protocol to ensure that everything works properly.\
The client must send the server the following information:
 - Filesize: 8-byte int
 - Chunk size: 8-byte int. The client can decide what an appropriate chunk size is.
 - Filename 21chars(bytes): 20-character string (+ string terminator).
 - The file contents.


## Transfers Queue
Since the active transfers list is dynamically growing/shrinking, a linked data structure is used. It is protected by a mutex and a condition variable. The list is modified by transfer threads, and is read by the UI thread and main thread.
The synchronization would work as follows:
 - When a transfer starts, a transfer thread addsits transfer record to the list
 - When a transfer ends, a worker thread removes its transfer record from thel ist
 - When the user asks to display active transfers, the UI thread iterates through the list and displays active transfer information
 - When the user asks to exit, UI thread sets the exit flag and terminates. The main thread reads the exit flag and terminates the whole program.


