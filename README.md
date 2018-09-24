# katchat 

[^._.^]ﾉ彡
Welcome to katchat, a simple chat server built entirely in C++.

Description
-----
Multichat server using TCP sockets and pthreads to handle new connections. The main loop sets up the socket and continously listens for new connections. When a new client connection is made, a new thread is created to handle that individual client.

```
int main() {
	//create sockets
	int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	//bind socket
	bind(SocketFD,(struct sockaddr *)&sa, sizeof sa)

	//listen for incoming connection
	listen(SocketFD, 10)

	//loop continously to listen for clients
	for(;;) {

		//accept client connection
		int ConnectFD = accept(SocketFD, NULL, NULL);

		//create new thread
		pthread_t thread;
   	 	pthread_create(&thread, NULL, handle_client, (void*) ConnectFD);
	}
}
```

The ```handle_client``` method continously reads commands from the individual client until the connection is closed. The client must first enter a unique username before proceeding. Then, the client has the option of viewing and joining various chat rooms to talk.

```
void* handle_client() { 
	//read commands until connection closes
	while(1) {

		//read message from client
		retval = recv(ConnectFD, buff, sizeof(buff), 0);

		//get username if not logged in
		if(!loggedin) {}

		//take normal commands
		else {
			//handle requests from outside chat room
			if(!inchat) {}

			//handle requests from inside chat room
			else {}
		}
	}
}
```

Clients can communicate with each other in chat rooms. Each ```chat_room``` object saves a list of file descriptors for each client in the room. When one client sends something to the server, the server can broadcast that message to the rest of the clients in the room by sending it through the file descriptor list.


Usage
-----

#### Compile and Run:
```
> $ make
g++ -pthread -Wall katchat.cpp -o katchat

> $ ./katchat 30666
```

#### Connecting Clients:

Each client must give a unique username in order to log into katchat.

* telnet:
```
> $ telnet 127.0.0.1 30666
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
[^._.^]ﾉ彡 Welcome to katchat!
Please enter a username.
> katherine
Sorry, username taken.
> sam
Welcome sam.
```

#### Commands:
All commands begin with a forward slash "/".

- ```/rooms```: show all active chat rooms
- ```/join chatty_kathy```: join the chat room named *chatty_kathy*
- ```/leave```: leave a chat room
- ```/help```: get help
- ```/quit```: close katchat

#### Chatting:

Use ```/join``` to join a chat room and just start talking! 

Try typing "katchat" in a chat room!







