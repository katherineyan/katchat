# katchat

Welcome to katchat, a simple chat server built entirely in C++.

Description
-----



Usage
-----

#### Compile and Run:
```
> $ make
g++ -Wall katchat.cpp -o katchat

> $ ./katchat 30666
```

#### Connecting Clients:

* telnet:
```
> $ telnet 127.0.0.1 30666
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
Welcome to katchat!
Please login to continue.
```

#### Commands:
All commands begin with a forward slash "/".

- ```/rooms```: show all active chat rooms
- ```/join title```: join the chat room named *title*
- ```/leave```: leave a chat room
- ```/quit```: close katchat
