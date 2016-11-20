

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>



int main(int argc, char * argv[]) {
    
    char buffer[256];
    
    //Check the command line arguments
    if (argc < 2) {
        printf ("Usage: server_port\n");
        return(0);
    }
    
    //Grab the port number from the command line
    int portNumber = atoi(argv[1]);
    
    
    //Struct for holding the C2C Server information and agent
    struct sockaddr_in c2c_addr;
    
    //Create a file descriptor for a socket
    int socketDiscriptor;
    if((socketDiscriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf (stderr, "ERROR: socket() failed\n");
        exit (-1);
    }
    
    //Fill in the information or the C2C Server
    c2c_addr.sin_family = AF_INET;
    c2c_addr.sin_port = htons(portNumber);
    c2c_addr.sin_addr.s_addr = INADDR_ANY;
    
    //Bind the socket to the C2C Server's IP and Port
    if((bind(socketDiscriptor, (struct sockaddr *) &c2c_addr, sizeof(c2c_addr))) < 0) {
        fprintf(stderr, "ERROR: bind() failed\n");
        exit(-1);
    }
    
    //Create queue of size 5 and sleep until an agent connects
    listen(socketDiscriptor, 5);
    
    //Run the program and exit with ctrl-c
    while(1) {
       
	//Structs and FD for every agent that listens
        struct sockaddr_in agent_addr;
        socklen_t agent_length;
	int agentSocketFD;
	bzero(buffer, 256);
 
        //Create a new socket for the agent
        agent_length = sizeof(agent_addr);
        if((agentSocketFD = accept(socketDiscriptor, (struct sockaddr*) &agent_addr, &agent_length)) < 0) {
            fprintf(stderr, "ERROR: accept failed\n");
            exit(-1);
        }

        //Start processing
        int n;
        if((n = read(agentSocketFD, buffer, 256)) < 0) {
            fprintf(stderr, "ERROR: Unable to read\n");
            exit(-1);
        }

        //Print the message
        printf("Here is the message: %s\n",buffer);

        //Write back to the client
	if ((n = write(agentSocketFD, "I got your message", 256)) < 0) {
            perror("ERROR writing to socket");
            exit(-1);
        }

	close(agentSocketFD);
    }
    
    return 0;
}
