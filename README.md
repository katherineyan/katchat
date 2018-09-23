# katchat [^._.^]ﾉ彡

Welcome to katchat, a simple chat server built entirely in C++.

Description
-----
Multichat server using TCP sockets and pthreads to handle new connections. The main loop sets up the socket and continously listens for new connections. When a new client connection is made, a new thread is created to handle that individual client.

The ```handle_client``` method continously reads commands from the individual client until the connection is closed. The client must first enter a unique username before proceeding. Then, the client has the option of viewing and joining various chat rooms to talk.


Usage
-----

#### Compile and Run:
```
> $ make
g++ -Wall katchat.cpp -o katchat

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
Welcome to katchat! [^._.^]ﾉ彡
Please login to continue.
sam
Welcome sam.
```

#### Commands:
All commands begin with a forward slash "/".

- ```/rooms```: show all active chat rooms
- ```/join chatty_kathy```: join the chat room named *chatty_kathy*
- ```/leave```: leave a chat room
- ```/quit```: close katchat

#### Limitations:
- ```MAX_NUM_CLIENTS```: indicates the max number of client connections at one time. Set at 1000.

#### Future Improvements:
- Private chat.
- Allow users to create their own chat rooms. 
- Web client.