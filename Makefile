all : Server Client

Server : Server.c
	gcc -o Server Server.c
Client : Client.c
	gcc -o Client Client.c

clean:
	rm Client Server
