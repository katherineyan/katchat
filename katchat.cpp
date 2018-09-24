/*
 * 
 * Katherine Yan
 *
 * katchat server implementation
 * 
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <iostream>
#include <sys/wait.h>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <vector>
#include <algorithm>
#include <pthread.h>
using namespace std;


/*----------- CHAT ROOM CLASS ------------*/
//chat room class
class chat_room {
  vector<string> usernames;
  vector<int> fds;
  string title;
  int num_users;
public:
  //constructors
  chat_room(vector<string> unames, vector<int> f, string t): usernames(unames), fds(f), title(t), num_users(unames.size()) {}
  chat_room(string t): title(t), num_users(0) {}
  //getters
  string get_title() const { return title; }
  int get_num_users() const { return num_users; }
  vector<string> get_usernames() const { return usernames; }
  vector<int> get_fds() const { return fds; }
  //methods
  int add_user_name(string u) { 
    usernames.push_back(u);
    num_users += 1; 
    return usernames.size() - 1;
  }
  int add_user_fd(int fd) {
    fds.push_back(fd);
    return fds.size() - 1;
  }
  void remove_user(int i1, int i2) {
    usernames.erase(usernames.begin() + i1);
    fds.erase(fds.begin() + i2);
    num_users -= 1;
  }
};

//to get a match on a chat room title
struct MatchString
{
 MatchString(const std::string& s) : s_(s) {}
 bool operator()(const chat_room& obj) const
 {
   return obj.get_title() == s_;
 }
 private:
   const std::string& s_;
};


/*----------- OTHER GLOBAL VARIABLES ------------*/
//message buffer
char msg[1024];
//all users
vector<string> users;
//all chat rooms
vector<chat_room> c_rooms;


/*----------- THREAD HELPER METHOD ------------*/
//handle individual client requests
void* handle_client(void* arg) {

  //cast to thread_arg 
  int ConnectFD = (intptr_t) arg;

  //name of user
  string name;
  //messsage buffer
  char buff[1024];
  //logged in or not
  bool loggedin = false;
  //in chatroom or not
  bool inchat = false;
  //index of user within chatroom usernames vector
  int chat_index;
  //index of user within chatroom fds vector
  int fd_index;
  //pointer to chatroom youre in
  chat_room* currchat;

  //send success message to client
  sprintf(buff, "%c[%dm%c[%dm[^._.^]ﾉ彡 Welcome to katchat!\r\nPlease enter a username.%c[%dm\r\n", 0x1B, 1, 0x1B, 7, 0x1b, 0); 
  int retval = send(ConnectFD, buff, strlen(buff), 0);
  if (retval < 0) {
    perror("Error, send failed.");
    close(ConnectFD);
    exit(EXIT_FAILURE);
  }

  /*----------- READING COMMANDS FROM CLIENT ------------*/
    
  while(1) {

    //read message from client to get username
    memset(&buff, 0, sizeof(buff)); //clear message buffer
    retval = recv(ConnectFD, buff, sizeof(buff), 0);
    if (retval == -1) {
      perror("Error, reading failed.");
      close(ConnectFD);
      exit(EXIT_FAILURE);
    }

    /*----------- NOT LOGGED IN ------------*/
    if (!loggedin) {
      string n = string(buff); 
      if (!n.empty()) {
        n.erase(n.size() - 1);
        n.erase(n.size() - 1);
      }
      name = n;
          
      if (count(users.begin(), users.end(), name)) {
        sprintf(buff, "%c[%dm%c[%dmSorry, username taken.%c[%dm\r\n", 0x1B, 1, 0x1B, 7, 0x1b, 0);
      }
      else {
        sprintf(buff, "%c[%dm%c[%dmWelcome %s.%c[%dm\r\n", 0x1B, 1, 0x1B, 7, name.c_str(), 0x1b, 0);
        users.push_back(name); //add name to user list
        loggedin = true;
      }  
      send(ConnectFD, buff, sizeof(buff), 0);
    }

    /*----------- LOGGED IN ------------*/
    else {

      /*----------- NOT IN A CHAT ROOM ------------*/
      if (!inchat) {

        //rooms: show active rooms 
        if(strncmp(buff, "/rooms", 6) == 0) {
          //no rooms
          if (c_rooms.empty()) { 
            memset(&buff, 0, sizeof(buff));
            sprintf(buff, "%c[%dm%c[%dmThere are currently no active rooms.%c[%dm\r\n", 0x1B, 1, 0x1B, 7, 0x1b, 0);
            send(ConnectFD, buff, sizeof(buff), 0);
          }
          //rooms exist
          else {
            memset(&buff, 0, sizeof(buff));
            sprintf(buff, "%c[%dm%c[%dmActive rooms are:%c[%dm\r\n", 0x1B, 1, 0x1B, 7, 0x1b, 0);
            send(ConnectFD, buff, sizeof(buff), 0);
            for(vector<chat_room>::const_iterator i = c_rooms.begin(); i != c_rooms.end(); ++i) {
              memset(&buff, 0, sizeof(buff)); //clear message buffer
              sprintf(buff, "%c[%dm%c[%dm* %s (%d)%c[%dm\r\n", 0x1B, 1, 0x1B, 7, i->get_title().c_str(), i->get_num_users(), 0x1b, 0);
              send(ConnectFD, buff, sizeof(buff), 0);
            }
            memset(&buff, 0, sizeof(buff)); //clear message buffer
            sprintf(buff, "%c[%dm%c[%dmend of list%c[%dm\r\n", 0x1B, 1, 0x1B, 7, 0x1b, 0);
            send(ConnectFD, buff, sizeof(buff), 0);
          } 
        }

        //join: join an active room
        else if(strncmp(buff, "/join", 5) == 0) {
          //get requested room name
          string roomname = string(&buff[6]);
          if (!roomname.empty()) {
            roomname.erase(roomname.size() - 1);
            roomname.erase(roomname.size() - 1);
          }
          //attempt to enter room
          vector<chat_room>::iterator it = find_if(c_rooms.begin(), c_rooms.end(), MatchString(roomname));
          if (it != c_rooms.end()) {  //if we can enter the room
            //send success message to client
            memset(&buff, 0, sizeof(buff));
            sprintf(buff, "%c[%dm%c[%dmentering room: %s%c[%dm\r\n", 0x1B, 1, 0x1B, 7, roomname.c_str(), 0x1b, 0);
            send(ConnectFD, buff, sizeof(buff), 0);
            //set currchat to point to room
            int index = distance(c_rooms.begin(), it);
            currchat = &c_rooms[index];
            //send enter message to other clients in room
            vector<int> fds = currchat->get_fds();
            for(vector<int>::iterator i = fds.begin(); i != fds.end(); ++i) {
              memset(&buff, 0, sizeof(buff)); 
              sprintf(buff, "%c[%dm%c[%dm* new user joined chat: %s%c[%dm\r\n", 0x1B, 1, 0x1B, 7, name.c_str(), 0x1b, 0);
              send(*i, buff, sizeof(buff), 0);
            }
            //add new user to the chat_room object
            chat_index = currchat->add_user_name(name);
            fd_index = currchat->add_user_fd(ConnectFD);
            inchat = true;
            //print all people in room
            vector<string> temp = currchat->get_usernames();
            for(vector<string>::const_iterator i = temp.begin(); i != temp.end(); ++i) {
              if(i->compare(name)) { //if its you
                memset(&buff, 0, sizeof(buff));
                sprintf(buff, "%c[%dm%c[%dm* %s%c[%dm\r\n", 0x1B, 1, 0x1B, 7, i->c_str(), 0x1b, 0);
                send(ConnectFD, buff, sizeof(buff), 0);
              }
              else {
                memset(&buff, 0, sizeof(buff)); //if its not you
                sprintf(buff, "%c[%dm%c[%dm* %s (**this is you)%c[%dm\r\n", 0x1B, 1, 0x1B, 7, i->c_str(), 0x1b, 0);
                send(ConnectFD, buff, sizeof(buff), 0);
              }      
            }
            //end of list
            memset(&buff, 0, sizeof(buff)); 
            sprintf(buff, "%c[%dm%c[%dmend of list%c[%dm\r\n", 0x1B, 1, 0x1B, 7, 0x1b, 0);
            send(ConnectFD, buff, sizeof(buff), 0);
          }
          //room doesn't exist
          else {
            memset(&buff, 0, sizeof(buff));
            sprintf(buff, "%c[%dm%c[%dmRoom %s doesn't exist.%c[%dm\r\n", 0x1B, 1, 0x1B, 7, roomname.c_str(), 0x1b, 0);
            send(ConnectFD, buff, sizeof(buff), 0);
          }
        }

        //quit: terminate connection
        else if(strncmp(buff, "/quit", 5) == 0) {
          memset(&buff, 0, sizeof(buff));
          sprintf(buff, "%c[%dm%c[%dmCya.%c[%dm\r\n", 0x1B, 1, 0x1B, 7, 0x1b, 0);
          send(ConnectFD, buff, sizeof(buff), 0);
          close(ConnectFD);
          break;
        }

        //help: show info
        else if(strncmp(buff, "/help", 5) == 0) {
          memset(&buff, 0, sizeof(buff));
          sprintf(buff, "%c[%dm%c[%dmUsername: %s.\r\nUse /rooms to see list of available chat rooms.\r\nUse /join to join a chat room.\r\nUse /quit to close the connection.%c[%dm\r\n", 0x1B, 1, 0x1B, 7, name.c_str(), 0x1b, 0);
          send(ConnectFD, buff, sizeof(buff), 0);
        }

        //leave: error message
        else if(strncmp(buff, "/leave", 6) == 0) {
          memset(&buff, 0, sizeof(buff));
          sprintf(buff, "%c[%dm%c[%dmYou're not in a chat room right now.%c[%dm\r\n", 0x1B, 1, 0x1B, 7, 0x1b, 0);
          send(ConnectFD, buff, sizeof(buff), 0);
        }

        //unknown command
        else {
          memset(&buff, 0, sizeof(buff));
          sprintf(buff, "%c[%dm%c[%dmUnknown command.%c[%dm\r\n", 0x1B, 1, 0x1B, 7, 0x1b, 0);
          send(ConnectFD, buff, sizeof(buff), 0);
        }

      }

      /*----------- IN A CHAT ROOM------------*/
      else {

        //leave: leaving room
        if(strncmp(buff, "/leave", 6) == 0) { 
          //send messsage about leaving
          vector<int> fds = currchat->get_fds();
          for(vector<int>::iterator i = fds.begin(); i != fds.end(); ++i) {
            if(*i == ConnectFD) { //if its you
              memset(&buff, 0, sizeof(buff));
              sprintf(buff, "%c[%dm%c[%dm* user has left chat: %s (**this is you)%c[%dm\r\n", 0x1B, 1, 0x1B, 7, name.c_str(), 0x1b, 0);
              send(ConnectFD, buff, sizeof(buff), 0);
            }
            else {
              memset(&buff, 0, sizeof(buff)); //if its not you
              sprintf(buff, "%c[%dm%c[%dm* user has left chat: %s%c[%dm\r\n", 0x1B, 1, 0x1B, 7, name.c_str(), 0x1b, 0);
              send(*i, buff, sizeof(buff), 0);
            } 
          }
          currchat->remove_user(chat_index, fd_index);
          inchat = false;
        }

        //join: error 
        else if(strncmp(buff, "/join", 5) == 0) { 
          memset(&buff, 0, sizeof(buff)); //clear message buffer
          sprintf(buff, "%c[%dm%c[%dmYou're already in a chat room.%c[%dm\r\n", 0x1B, 1, 0x1B, 7, 0x1b, 0);
          send(ConnectFD, buff, sizeof(buff), 0);
        }

        //rooms: error 
        else if(strncmp(buff, "/rooms", 6) == 0) { 
          memset(&buff, 0, sizeof(buff)); //clear message buffer
          sprintf(buff, "%c[%dm%c[%dmLeave the chat room to see all available rooms.%c[%dm\r\n", 0x1B, 1, 0x1B, 7, 0x1b, 0);
          send(ConnectFD, buff, sizeof(buff), 0);
        }

        //help: show help
        else if(strncmp(buff, "/help", 5) == 0) { 
          memset(&buff, 0, sizeof(buff));
          sprintf(buff, "%c[%dm%c[%dmUsername: %s.\r\nCurrent chat room: %s.\r\nStart typing to talk to other people in the chat!\r\nUse /leave to leave the chat room.%c[%dm\r\n", 0x1B, 1, 0x1B, 7, name.c_str(), currchat->get_title().c_str(), 0x1b, 0);
          send(ConnectFD, buff, sizeof(buff), 0);
        }

        //quit: error
        else if(strncmp(buff, "/quit", 5) == 0) { 
          memset(&buff, 0, sizeof(buff)); //clear message buffer
          sprintf(buff, "%c[%dm%c[%dmYou must exit the room before you quit.%c[%dm\r\n", 0x1B, 1, 0x1B, 7, 0x1b, 0);
          send(ConnectFD, buff, sizeof(buff), 0);
        }

        //unknown command (starts with a "/")
        else if (strncmp(buff, "/", 1) == 0){
          memset(&buff, 0, sizeof(buff));
          sprintf(buff, "%c[%dm%c[%dmUnknown command.%c[%dm\r\n", 0x1B, 1, 0x1B, 7, 0x1b, 0);
          send(ConnectFD, buff, sizeof(buff), 0);
        }

        //just saying stuff
        else {
          //copy message and get fds
          char buff2[1024];
          strcpy(buff2, buff);
          vector<int> fds = currchat->get_fds();
          //send enter message to other clients in room
          memset(&buff, 0, sizeof(buff));
          sprintf(buff, "%s: %s", name.c_str(), buff2);
          for(vector<int>::iterator i = fds.begin(); i != fds.end(); ++i) {
            send(*i, buff, sizeof(buff), 0);
          }
          //katchat cat
          if (strncmp(buff2, "katchat", 7) == 0){
            for(vector<int>::iterator i = fds.begin(); i != fds.end(); ++i) {
              memset(&buff, 0, sizeof(buff));
              sprintf(buff, "%c[%dm%c[%dm[^._.^]ﾉ彡 meow%c[%dm\r\n", 0x1B, 1, 0x1B, 7, 0x1b, 0);
              send(*i, buff, sizeof(buff), 0);
            }
          }
        }
      }
    }
  }
}


/*----------- MAIN METHOD ------------*/
int main(int argc, char** argv) {

  /*----------- BASE USERS AND ROOMS ------------*/
  //empty vector
  vector<int> fds;
  fds.push_back(-1);
  //fake lists for rooms
  vector<string> users1;
  vector<string> users2;
  vector<string> users3;
  //fake users
  users.push_back("katherine");
  users1.push_back("katherine");
  users.push_back("mario");
  users3.push_back("mario");
  users.push_back("gh");
  users2.push_back("gh");
  users.push_back("luigi");
  users3.push_back("luigi");
  users.push_back("bowser");
  users3.push_back("bowser");
  users.push_back("link");
  users3.push_back("link");
  users.push_back("zelda");
  users3.push_back("zelda");
  users.push_back("isaac");
  users2.push_back("isaac");
  users.push_back("apollo");
  users2.push_back("apollo");
  users.push_back("gizmo");
  users2.push_back("gizmo");
  //fake chat rooms
  chat_room chat1(users1, fds, "chatty_kathy");
  chat_room chat2(users2, fds, "hottub");
  chat_room chat3(users3, fds, "super_smash_bros");
  c_rooms.push_back(chat1);
  c_rooms.push_back(chat2);
  c_rooms.push_back(chat3);


	/*----------- SETUP ------------*/
	//make sure to get port number
	if(argc != 2) {
	perror("Error, no port provided.\r\n");
      exit(0);
  }
  int port = atoi(argv[1]);

  //setup socket, IP, and ports
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;  
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port = htons(port); 

  //create socket
  int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (SocketFD < 0) {
  	perror("Error creating server socket.\r\n");
  	exit(EXIT_FAILURE);
  }
  
  //bind socket to local address
  if (bind(SocketFD,(struct sockaddr *)&sa, sizeof sa) < 0) {
      perror("Error, binding failed.\r\n");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }

  //listen for incoming connections
  if (listen(SocketFD, 10) < 0) {
    perror("Error listening for connection.\r\n");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

  /*----------- LOOP CONTINOUSLY TO LISTEN FOR CLIENTS ------------*/
  for (;;) {

  	//accept to initialize connection
  	int ConnectFD = accept(SocketFD, NULL, NULL);
  	if (0 > ConnectFD) {
  		perror("Error, accept failed.\r\n");
  		close(SocketFD);
  		exit(EXIT_FAILURE);
    } 

    //create threads for each connection
    pthread_t thread;
    pthread_create(&thread, NULL, handle_client, (void*) ConnectFD);

  }

  // Close sockets and kill thread
  close(SocketFD);
  pthread_exit(NULL);
  return 0;
}












