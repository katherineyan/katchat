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
using namespace std;


int main(int argc, char** argv) {

	/*----------- SETUP ------------*/

	//make sure to get port number
	if(argc != 2) {
	perror("Error, no port provided");
      exit(0);
  }
  int port = atoi(argv[1]);

  //message buffer
  char msg[1024];

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

    // int count = 0;
    // binary = false;

  /*----------- LOOP CONTINOUSLY TO LISTEN ------------*/
  for (;;) {

  	//accept to initialize connection
  	int ConnectFD = accept(SocketFD, NULL, NULL);
  	if (0 > ConnectFD) {
  		perror("Error, accept failed");
  		close(SocketFD);
  		exit(EXIT_FAILURE);
    } 

  	//send success message to client 
    strcpy(msg, "Welcome to katchat\r\n");
  	int retval = send(ConnectFD, msg, strlen(msg), 0);
  	if (retval < 0) {
  		perror("Error, send failed");
      close(ConnectFD);
      close(SocketFD);
      exit(EXIT_FAILURE);
    }

    /*----------- READING COMMANDS FROM CLIENT ------------*/
    //start reading from client
    while(1) {

      //read message from client 
      memset(&msg, 0, sizeof(msg)); //clear message buffer
      retval = recv(ConnectFD, msg, sizeof(msg), 0);
      if (retval == -1) {
      	perror("Error, reading failed");
      	close(ConnectFD);
      	close(SocketFD);
      	exit(EXIT_FAILURE);
      }

      /*----------- COMMANDS ------------*/

      //quit: terminate command connection
      if(strncmp(msg, "quit", 4) == 0) {
        strcpy(msg, "Server closing control connection.\r\n");
        send(ConnectFD, msg, sizeof(msg), 0);
        exit(EXIT_sSUCCESS);
      }

      //something else:
      // else if(1 == 1) {}




    }
  }

}












