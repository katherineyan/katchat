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
using namespace std;


/*----------- CHAT ROOM CLASS ------------*/
//chat room class
class chat_room {
  vector<string> usernames;
  string title;
  int num_users;

public:
  chat_room(vector<string> unames, string t): usernames(unames), title(t), num_users(unames.size()) {}
  chat_room(string t): title(t), num_users(0) {}
  string get_title() const { return title; }
  int get_num_users() const { return num_users; }
  vector<string> get_usernames() const { return usernames; }
  void add_user(string u) { 
    usernames.push_back(u);
    num_users += 1; 
  }
  // bool operator==(const chat_room &a, const chat_room &b) {
  //   return !a.get_title().compare(b.get_title());
  // }
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


/*----------- THREADS ------------*/
//thread struct
struct thread_arg {
  string name;
  int id;
  int SocketFD;
  int ConnectFD;
};

//hold the threads and thread_args of each client connection
int MAX_NUM_CLIENTS = 1000;
int CURR_NUM_CLIENTS = 0;
thread_arg arg[1000];
pthread_t threads[1000];


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
  thread_arg* t = (thread_arg*) arg;
  cout << t->id << endl;

  //messsage buffer
  char buff[1024];
  //logged in or not
  bool loggedin = false;
  //in chatroom or not
  bool inchat = false;
  chat_room* currchat;

  //send success message to client 
  strcpy(buff, "Welcome to katchat!\r\nPlease login to continue.\r\n");
  int retval = send(t->ConnectFD, buff, strlen(buff), 0);
  if (retval < 0) {
    perror("Error, send failed.");
    close(t->ConnectFD);
    close(t->SocketFD);
    exit(EXIT_FAILURE);
  }

  /*----------- READING COMMANDS FROM CLIENT ------------*/
    
  while(1) {

    //read message from client to get username
    memset(&buff, 0, sizeof(buff)); //clear message buffer
    retval = recv(t->ConnectFD, buff, sizeof(buff), 0);
    if (retval == -1) {
      perror("Error, reading failed.");
      close(t->ConnectFD);
      close(t->SocketFD);
      exit(EXIT_FAILURE);
    }

    /*----------- NOT LOGGED IN ------------*/
    if (!loggedin) {
      t->name = string(buff); 
      if (count(users.begin(), users.end(), t->name)) {
        strcpy(buff, "Sorry, username taken.\r\n");
      }
      else {
        sprintf(buff, "Welcome %s", t->name.c_str());
        users.push_back(t->name); //add name to user list
        loggedin = true;
      }  
      send(t->ConnectFD, buff, sizeof(buff), 0);
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
            strcpy(buff, "There are currently no active rooms.\r\n");
            send(t->ConnectFD, buff, sizeof(buff), 0);
          }
          //rooms exist
          else {
            memset(&buff, 0, sizeof(buff));
            strcpy(buff, "Active rooms are:\r\n");
            send(t->ConnectFD, buff, sizeof(buff), 0);
            for(vector<chat_room>::const_iterator i = c_rooms.begin(); i != c_rooms.end(); ++i) {
              memset(&buff, 0, sizeof(buff)); //clear message buffer
              sprintf(buff, "* %s (%d)\r\n", i->get_title().c_str(), i->get_num_users());
              send(t->ConnectFD, buff, sizeof(buff), 0);
            }
            memset(&buff, 0, sizeof(buff)); //clear message buffer
            strcpy(buff, "end of list\r\n");
            send(t->ConnectFD, buff, sizeof(buff), 0);
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
          if (it != c_rooms.end()) {
            //update info about room
            int index = distance(c_rooms.begin(), it);
            currchat = &c_rooms[index];
            currchat->add_user(t->name);
            cout << currchat->get_title() << endl;
            cout << currchat->get_num_users() << endl;
            vector<string> temp = currchat->get_usernames();
            for (vector<string>::const_iterator i = temp.begin(); i != temp.end(); ++i)
              cout << *i << ' ';
            //print all people in room
            // for(vector<chat_room>::const_iterator i = c_rooms.begin(); i != c_rooms.end(); ++i) {
            //   memset(&buff, 0, sizeof(buff)); //clear message buffer
            //   sprintf(buff, "* %s (%d)\r\n", i->get_title().c_str(), i->get_num_users());
            //   send(t->ConnectFD, buff, sizeof(buff), 0);
            // }

          }
          //room doesn't exist
          else {
            memset(&buff, 0, sizeof(buff));
            sprintf(buff, "Room %s doesn't exist.\r\n", roomname.c_str());
            send(t->ConnectFD, buff, sizeof(buff), 0);
          }

          
          // //attempt to enter room
          // auto it = find_if(c_rooms.begin(), c_rooms.end(), [&roomname](chat_room curr) {
          //   return !(curr.get_title() == roomname);
          // });

          // cout << it->get_title() << endl;



          // vector<chat_room>::iterator it;
          // it = find_if(c_rooms.begin(), c_rooms.end(), chat_room(string(roomname)));
          // if(it != c_rooms.end()) { //enter
          //   memset(&buff, 0, sizeof(buff));
          //   sprintf(buff, "Entering room: %s", roomname);
          //   send(t->ConnectFD, buff, sizeof(buff), 0);
          //   inchat = true;
          //   //enter room
          //   //update list and num of people
          //   //print list of people
          // }




        }

        //quit: terminate connection
        else if(strncmp(buff, "/quit", 5) == 0) {
          memset(&buff, 0, sizeof(buff));
          strcpy(buff, "Server closing control connection.\r\n");
          send(t->ConnectFD, buff, sizeof(buff), 0);
          // close(t->SocketFD);
          // close(t->ConnectFD);
          // exit(EXIT_SUCCESS); !!!!!!!!!exits the server, need to exit client
        }

        //leave: error message
        else if(strncmp(buff, "/leave", 6) == 0) {
          memset(&buff, 0, sizeof(buff));
          strcpy(buff, "You're not in a chat room right now.\r\n");
          send(t->ConnectFD, buff, sizeof(buff), 0);
        }

        //unknown command
        else {
          memset(&buff, 0, sizeof(buff));
          strcpy(buff, "Unknown command.\r\n");
          send(t->ConnectFD, buff, sizeof(buff), 0);
        }

      }

      /*----------- IN A CHAT ROOM------------*/
      else {

        //leave: leaving room
        if(strncmp(buff, "/leave", 6) == 0) { 
          //edit room variables

          memset(&buff, 0, sizeof(buff)); //clear message buffer
          sprintf(buff, "* user has left chat: %s", t->name.c_str()); //how to broadcast to whole chat?
          send(t->ConnectFD, buff, sizeof(buff), 0);
          inchat = false;
        }

        //join/rooms: error 
        else if((strncmp(buff, "/join", 5) == 0) || (strncmp(buff, "/rooms", 6) == 0) ) { 
          memset(&buff, 0, sizeof(buff)); //clear message buffer
          strcpy(buff, "You're already in a chat room.\r\n");
          send(t->ConnectFD, buff, sizeof(buff), 0);
        }

        //quit: error
        else if(strncmp(buff, "/quit", 5) == 0) { 
          memset(&buff, 0, sizeof(buff)); //clear message buffer
          strcpy(buff, "You must exit the room before you quit.\r\n");
          send(t->ConnectFD, buff, sizeof(buff), 0);
        }

        //unknown command (starts with a "/")
        else if (strncmp(buff, "/", 1) == 0){
          memset(&buff, 0, sizeof(buff));
          strcpy(buff, "Unknown command.\r\n");
          send(t->ConnectFD, buff, sizeof(buff), 0);
        }

        // //just saying stuff
        // else {
        //   //need to broadcast to all chat????
        //   int temp = 1;
        // }
        
      }
    }
  }
}


/*----------- MAIN METHOD ------------*/
int main(int argc, char** argv) {

  /*----------- BASE USERS AND ROOMS ------------*/
  //fake lists for rooms
  vector<string> users1;
  vector<string> users2;
  vector<string> users3;
  //fake users
  users.push_back("katherine\r\n");
  users1.push_back("katherine\r\n");
  users.push_back("mario\r\n");
  users3.push_back("mario\r\n");
  users.push_back("gh\r\n");
  users2.push_back("gh\r\n");
  users.push_back("luigi\r\n");
  users3.push_back("luigi\r\n");
  users.push_back("bowser\r\n");
  users3.push_back("bowser\r\n");
  users.push_back("link\r\n");
  users3.push_back("link\r\n");
  users.push_back("zelda\r\n");
  users3.push_back("zelda\r\n");
  users.push_back("isaac\r\n");
  users2.push_back("isaac\r\n");
  users.push_back("apollo\r\n");
  users2.push_back("apollo\r\n");
  users.push_back("gizmo\r\n");
  users2.push_back("gizmo\r\n");
  //fake chat rooms
  chat_room chat1(users1, "chatty_kathy");
  chat_room chat2(users2, "hottub");
  chat_room chat3(users3, "super_smash_bros");
  c_rooms.push_back(chat1);
  c_rooms.push_back(chat2);
  c_rooms.push_back(chat3);


	/*----------- SETUP ------------*/
	//make sure to get port number
	if(argc != 2) {
	perror("Error, no port provided.");
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
  	perror("Error creating server socket.");
  	exit(EXIT_FAILURE);
  }
  
  //bind socket to local address
  if (bind(SocketFD,(struct sockaddr *)&sa, sizeof sa) < 0) {
      perror("Error, binding failed.");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }

  //listen for incoming connections
  if (listen(SocketFD, 10) < 0) {
    perror("Error listening for connection.");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

  /*----------- LOOP CONTINOUSLY TO LISTEN FOR CLIENTS ------------*/
  for (;;) {

  	//accept to initialize connection
  	int ConnectFD = accept(SocketFD, NULL, NULL);
  	if (0 > ConnectFD) {
  		perror("Error, accept failed.");
  		close(SocketFD);
  		exit(EXIT_FAILURE);
    } 

    //create new thread for the connection
    if (CURR_NUM_CLIENTS < MAX_NUM_CLIENTS - 1) {
      arg[CURR_NUM_CLIENTS].id = CURR_NUM_CLIENTS;
      arg[CURR_NUM_CLIENTS].SocketFD = SocketFD;
      arg[CURR_NUM_CLIENTS].ConnectFD = ConnectFD;
      pthread_create(&threads[CURR_NUM_CLIENTS], NULL, handle_client, &arg[CURR_NUM_CLIENTS]);
      CURR_NUM_CLIENTS += 1;
    }
    //else if we've reached the max number of people
    else {
      strcpy(msg, "Sorry, the max number of people on katchat has been reached.\r\n");
      int retval = send(ConnectFD, msg, strlen(msg), 0);
      if (retval < 0) {
        perror("Error, send failed.");
        close(ConnectFD);
        close(SocketFD);
        exit(EXIT_FAILURE);
      }
    }
  }

  return 1;
}












