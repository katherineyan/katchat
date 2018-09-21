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
  	exit(0);
  }
  
  //bind socket to local address
  int bindStatus = bind(SocketFD,(struct sockaddr *)&sa, sizeof(sa));
  if (bindStatus < 0) {
  	perror("Error binding socket");
  	exit(0);
  }

  //listen for incoming connections
  if (listen(SocketFD, 10) < 0) {
    perror("Error listening for connection");
    close(SocketFD);
    exit(0);
  }

    // int count = 0;
    // binary = false;

  /*----------- LOOP CONTINOUSLY ------------*/
  for (;;) {

  	//accept to initialize connection
  	int ConnectFD = accept(SocketFD, NULL, NULL);
  	if (0 > ConnectFD) {
  		perror("Error, accept failed");
  		close(SocketFD);
  		exit(0);
    } 

  	//send success message to client 
  	char msg[] = "Welcome to katchat\r\n";
  	int retval = send(ConnectFD, msg, strlen(msg), 0);
  	if (retval < 0) {
  		perror("Error, send failed");
      close(ConnectFD);
      close(SocketFD);
      exit(EXIT_FAILURE);
    }

    //start reading from client
    while(1) {

    	//setup buffer for reading 
    	char buffer[1024];

      //read message from client 
      retval = recv(ConnectFD, buffer, sizeof(buffer), 0);
      if (retval == -1) {
      	perror("Error, reading failed");
      	close(ConnectFD);
      	close(SocketFD);
      	exit(0);
      }

      /*----------- COMMANDS ------------*/
    }
  }

}












