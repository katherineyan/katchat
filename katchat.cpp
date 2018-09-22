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
  chat_room(vector<string>, string);
  string get_title() const { return title;}
  int get_num_users() const {return num_users;}
};

chat_room::chat_room(vector<string> unames, string t) { //constructor
  usernames = unames;
  title = t;
  num_users = unames.size();
}

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

  //send success message to client 
  strcpy(buff, "Welcome to katchat\r\nPlease login to continue\r\n");
  int retval = send(t->ConnectFD, buff, strlen(buff), 0);
  if (retval < 0) {
    perror("Error, send failed");
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
      perror("Error, reading failed");
      close(t->ConnectFD);
      close(t->SocketFD);
      exit(EXIT_FAILURE);
    }

    //get username if not loggedin
    if (!loggedin) {
      t->name = string(buff); 
      if (count(users.begin(), users.end(), t->name)) {
        strcpy(buff, "Sorry, username taken\r\n");
      }
      else {
        sprintf(buff, "Welcome %s", t->name.c_str());
        users.push_back(t->name); //add name to user list
        loggedin = true;
      }  
      send(t->ConnectFD, buff, sizeof(buff), 0);
    }

    //if loggedin, handle normal requests
    else {

      //rooms: show active rooms 
      if(strncmp(buff, "/rooms", 6) == 0) {
        if (c_rooms.empty()) { //no rooms
          strcpy(buff, "There are currently no active rooms\r\n");
          send(t->ConnectFD, buff, sizeof(buff), 0);
        }
        else { //nonzero num rooms
          strcpy(buff, "Active rooms are:\r\n");
          send(t->ConnectFD, buff, sizeof(buff), 0);
          for(vector<chat_room>::const_iterator i = c_rooms.begin(); i != c_rooms.end(); ++i) {
            memset(&buff, 0, sizeof(buff)); //clear message buffer
            sprintf(buff, "* %s (%d)\r\n", i->get_title().c_str(), i->get_num_users());
            send(t->ConnectFD, buff, sizeof(buff), 0);
          }
        } 
      }



      //quit: terminate command connection
      else if(strncmp(buff, "/quit", 5) == 0) {
        strcpy(buff, "Server closing control connection.\r\n");
        send(t->ConnectFD, buff, sizeof(buff), 0);
        // exit(EXIT_SUCCESS); !!!!!!!!!exits the server, need to exit client
      }

      //unknown command
      else {
        strcpy(buff, "Unknown command.\r\n");
        send(t->ConnectFD, buff, sizeof(buff), 0);
      }

    }
  }
}


/*----------- MAIN METHOD ------------*/
int main(int argc, char** argv) {

	/*----------- SETUP ------------*/
	//make sure to get port number
	if(argc != 2) {
	perror("Error, no port provided");
      exit(0);
  }
  int port = atoi(argv[1]);

  //fake users
  users.push_back("katherine\r\n");
  users.push_back("mario\r\n");
  users.push_back("gh\r\n");
  users.push_back("luigi\r\n");
  users.push_back("bowser\r\n");
  users.push_back("link\r\n");
  users.push_back("zelda\r\n");
  users.push_back("isaac\r\n");
  users.push_back("apollo\r\n");
  users.push_back("gizmo\r\n");
  //fake chat rooms
  chat_room chat1(users, "CR1");
  chat_room chat2(users, "CR2");
  chat_room chat3(users, "CR3");
  c_rooms.push_back(chat1);
  c_rooms.push_back(chat2);
  c_rooms.push_back(chat3);

  
  //setup socket, IP, and ports
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;  
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port = htons(port); 

  //create socket
  int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (SocketFD < 0) {
  	perror("Error creating server socket");
  	exit(EXIT_FAILURE);
  }
  
  //bind socket to local address
  int bindStatus = bind(SocketFD,(struct sockaddr *)&sa, sizeof(sa));
  if (bindStatus < 0) {
  	perror("Error binding socket");
  	exit(EXIT_FAILURE);
  }

  //listen for incoming connections
  if (listen(SocketFD, 10) < 0) {
    perror("Error listening for connection");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

  /*----------- LOOP CONTINOUSLY TO LISTEN FOR CLIENTS ------------*/
  for (;;) {

  	//accept to initialize connection
  	int ConnectFD = accept(SocketFD, NULL, NULL);
  	if (0 > ConnectFD) {
  		perror("Error, accept failed");
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
      strcpy(msg, "Sorry, the max number of people on katchat has been reached\r\n");
      int retval = send(ConnectFD, msg, strlen(msg), 0);
      if (retval < 0) {
        perror("Error, send failed");
        close(ConnectFD);
        close(SocketFD);
        exit(EXIT_FAILURE);
      }
    }
  }

  return 1;
}












