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
  chat_room(vector<string>, string, int);
  // void print_users();
};

chat_room::chat_room(vector<string> unames, string t, int num) { //constructor
  usernames = unames;
  title = t;
  num_users = num;
}

// void chat_room::print_users() {
//   for(vector<string>::const_iterator i = usernames.begin(); i != usernames.end(); ++i) {
//     send(ConnectFD, *i, sizeof(*i), 0);
//   }
// }

/*----------- THREADS ------------*/

//thread struct
struct thread_arg {
  // string name;
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
//name buffer
string name;
//all users
vector<string> users;


/*----------- THREAD HELPER METHOD ------------*/
//handle individual client requests
void* handle_client(void* arg) {

  //cast to thread_arg 
  thread_arg* t = (thread_arg*) arg;

  cout << t->id << endl;

  //messsage buffer
  char buff[1024];

  //send success message to client 
  strcpy(buff, "Welcome to katchat\r\nPlease login to continue\r\n");
  int retval = send(t->ConnectFD, buff, strlen(buff), 0);
  if (retval < 0) {
    perror("Error, send failed");
    close(t->ConnectFD);
    close(t->SocketFD);
    exit(EXIT_FAILURE);
  }



}


int main(int argc, char** argv) {

	/*----------- SETUP ------------*/

	//make sure to get port number
	if(argc != 2) {
	perror("Error, no port provided");
      exit(0);
  }
  int port = atoi(argv[1]);

  
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
  //all chat rooms
  vector<chat_room> c_rooms;
  chat_room* chat;

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
      arg[CURR_NUM_CLIENTS].SocketFD = SocketFD;
      arg[CURR_NUM_CLIENTS].ConnectFD = ConnectFD;
      arg[CURR_NUM_CLIENTS].id = CURR_NUM_CLIENTS;
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












