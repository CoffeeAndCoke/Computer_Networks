

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


//Linked list to keep track of agents connected to the server
struct activeAgents {
    
    char IP[256]; //Stores the IP of the active agent
    time_t agentTime; //Stores the time at which the agent join the server
    struct activeAgents* next;
    
};

//Poiinters to the linked list containing the active agents
struct activeAgents* head = NULL;
//struct node *current = NULL;


//insert active agent at the start of the list with the time it joined
void insertActiveAgent(char* ip, time_t joinTime) {
    
    struct activeAgents *temp = malloc(sizeof(struct activeAgents));
    
    //Copy the IP and Time to the linked list
    strcpy(temp->IP, ip);
    temp->agentTime = joinTime;
    
    //point it to old first node
    temp->next = head;
    
    //point first to new first node
    head = temp;
}


void removeActiveAgent(char* ip) {
    
    struct activeAgents* ptr = head->next;
    struct activeAgents* prevPtr = head;
    
    //First check if the first node is getting delete
    //If so, delete it and point the head to the next node
    if(strcmp(prevPtr->IP, ip) == 0){
        head = ptr; //Point the head to node after the first
        free(prevPtr);
        return;
    } else {
        
        while(ptr) {
            
            if(strcmp(ptr->IP, ip) == 0) {
                prevPtr->next = ptr->next;
                free(ptr);
                return;
            }
            
            prevPtr = ptr;
            ptr = ptr->next;
        }
    }
    
    return;
}

//display all the active agents
void display() {
    
    struct activeAgents* ptr = head;
    int i;
    
    //start from the beginning
    while(ptr != NULL) {
        
        for(i = 0; ptr->IP[i] != '\0'; i++) {
            
            printf("Agent %d IP Address:  %c", i, ptr->IP[i]);
        }
        ptr = ptr->next;
    }
    
    printf("\n");
}

//Checks if an IP address is already in the list of Active Agents
int inList(char* ip) {
    
    int checkList = 0;
    
    //Point to the head for traversing
    struct activeAgents* ptr = head;
    
    //Traverse the List and check if the IP is on the list
    while(ptr) {
        
        if(strcmp(ptr->IP, ip) == 0) {
            checkList = 1;
        }
        
        ptr = ptr->next;
    }
    
    return checkList;
}



int main(int argc, char * argv[]) {
    
    //Check the command line arguments
    if (argc < 2) {
        printf ("Usage: server_port\n");
        return(0);
    }
    
    //Grab the port number from the command line
    int portNumber = atoi(argv[1]);
    
    //Used for comparing agent request
    char buffer[256];
    const char JOIN[] = "#JOIN";
    const char LEAVE[] = "#LEAVE";
    const char LIST[] = "#LIST";
    const char LOG[] = "#LOG";
    
    
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
    
    
    //Time tracking variables
    time_t serverStartTime; //Stores the server local start time
    time_t agentStartTime;
    time(&serverStartTime); //start time of server saved
    
    //Create queue of size 5 and sleep until an agent connects
    char printTime[32];
    listen(socketDiscriptor, 5);
    
    //Run the program and exit with ctrl-c
    while(1) {
        
        //CREATE A LOG FILE OR OPEN IT EACH ITERATION
        FILE *logFile = fopen("log.txt", "a+");
        if (logFile == NULL) {
            fprintf(stderr, "ERROR: Unable to LOG!\n");
            exit(-1);
        }
        
        
       
        //Structs and FD for every agent that listens
        struct sockaddr_in agent_addr;
        socklen_t agent_length;
        int agentSocketFD;
        bzero(buffer, 256); //Reset the Buffer to take in request
        
 
        //Create a new socket for the agent
        agent_length = sizeof(agent_addr);
        if((agentSocketFD = accept(socketDiscriptor, (struct sockaddr*) &agent_addr, &agent_length)) < 0) {
            fprintf(stderr, "ERROR: accept failed\n");
            exit(-1);
        }
        
        //Start processing the agent's request
        int n;
        if((n = read(agentSocketFD, buffer, 256)) < 0) {
            fprintf(stderr, "ERROR: Unable to read\n");
            exit(-1);
        }
        
        //READ IN THE REQUEST FROM THE AGENT AND RESPOND//
        //**********************************************//
        
        //COPY IP ADDRESS FROM AGENT
        char agentIP[8]; //Stores the IP of the agent trying to communicate
        strcpy(agentIP, inet_ntoa(agent_addr.sin_addr));
        
        //Agent Passed the JOIN Command
        if(strcmp(JOIN, buffer) == 0) {
            //The agent wants to join the server
            //Convert the Time, save the request to the log file and print to console
            time(&agentStartTime); //Record the time for the agent
            strcpy(printTime, ctime(&agentStartTime)); //Copy to the printTime array
            printTime[strlen(printTime) - 1] = '\0'; //Replace the new line
            fprintf(logFile, "%s: Received a “#JOIN” action from agent %s\n", printTime, agentIP);
            printf ("%s: Received a “#JOIN” action from agent %s\n", printTime, agentIP);
            
            //Agent isn't on the list
            if(inList(agentIP) == 0) {
                
                //Add IP of the agent to the list along with the time
                insertActiveAgent(agentIP, agentStartTime);
                
                //Respond to the client
                if ((n = write(agentSocketFD, "$OK", 256)) < 0) {
                    perror("ERROR writing to socket");
                    exit(-1);
                }
                
                //Save to log file and print to console
                fprintf(logFile, "%s: Responded to agent %s with $OK \n", printTime, agentIP);
                printf ("%s: Responded to agent %s with $OK \n", printTime, agentIP);
                
            } else {
                //Already on the list
                //Write back to the client
                if ((n = write(agentSocketFD, "$ALREADY MEMBER", 256)) < 0) {
                    perror("ERROR writing to socket");
                    exit(-1);
                }
                
                //Convert the time, save to log file and print to console
                fprintf(logFile, "%s: Responded to agent %s with $ALREADY_MEMBER\n", printTime, agentIP);
                printf ("%s: Responded to agent %s with $ALREADY_MEMBER\n", printTime, agentIP);
                
            }
        }
        else if (strcmp(LEAVE, buffer) == 0) {
            //The agent requested to leave the server
            //Convert the time, log to file and print to console
            time(&agentStartTime); //Record the time for the agent
            strcpy(printTime, ctime(&agentStartTime)); //Copy to the printTime array
            printTime[strlen(printTime) - 1] = '\0'; //Replace the new line
            fprintf(logFile, "%s: Received a “#LEAVE” action from agent %s\n", printTime, agentIP);
            printf ("%s: Received a “#LEAVE” action from agent %s\n", printTime, agentIP);
            
            //Check that the agent is in the list first
            if(inList(agentIP) == 1) {
                
                
                //Remove the agent with the given IP
                removeActiveAgent(agentIP);
                
                //Respond to the client
                if ((n = write(agentSocketFD, "$OK", 256)) < 0) {
                    perror("ERROR writing to socket");
                    exit(-1);
                }
                
                //Save to log file and print to console
                fprintf(logFile, "%s: Responded to agent %s with $OK \n", printTime, agentIP);
                printf ("%s: Responded to agent %s with $OK \n", printTime, agentIP);
                
            } else {
                
                //Not a member, can't run command
                if ((n = write(agentSocketFD, "$NOT MEMBER", 256)) < 0) {
                    perror("ERROR writing to socket");
                    exit(-1);
                }
                
                //Convert the time, save to log file and print to console
                fprintf(logFile, "%s: Responded to agent %s with $NOT_MEMBER\n", printTime, agentIP);
                printf ("%s: Responded to agent %s with $NOT_MEMBER\n", printTime, agentIP);
                
            }
         
        }
        else if (strcmp(LIST, buffer) == 0) {
            //Agent wants a list of all active agents
            //Convert the time, log to file and print to console
            time(&agentStartTime); //Record the time for the agent
            strcpy(printTime, ctime(&agentStartTime)); //Copy to the printTime array
            printTime[strlen(printTime) - 1] = '\0'; //Replace the new line
            fprintf(logFile, "%s: Received a “#LIST” action from agent %s\n", printTime, agentIP);
            printf ("%s: Received a “#LIST” action from agent %s\n", printTime, agentIP);
            bzero(printTime, 20); //Reset the time

            //Check that the agent is on the list
            if(inList(agentIP) == 1) {
                display(); //Display the Agent's in List
            } else {
                
                //The agent is not in list, ignore request
                if ((n = write(agentSocketFD, " ", 256)) < 0) {
                    perror("ERROR writing to socket");
                    exit(-1);
                }
                
                
                //Convert the time, save to log file and print to console
                fprintf(logFile, "%s: Responded to agent %s with " "\n", printTime, agentIP);
                printf ("%s: Responded to agent %s with " "\n", printTime, agentIP);
                
            }
        }
        else if (strcmp(LOG, buffer) == 0) {
            //Agent requested a log file
            //Convert the time, log to file and print to console
            time(&agentStartTime); //Record the time for the agent
            strcpy(printTime, ctime(&agentStartTime)); //Copy to the printTime array
            printTime[strlen(printTime) - 1] = '\0'; //Replace the new line
            fprintf(logFile, "%s: Received a “#LOG” action from agent %s\n", printTime, agentIP);
            printf ("%s: Received a “#LOG” action from agent %s\n", printTime, agentIP);
            bzero(printTime, 20); //Reset the time
        }
        else {
            
            //The agent passed an invalid command
            if ((n = write(agentSocketFD, "Invalid Command", 256)) < 0) {
                perror("ERROR writing to socket");
                exit(-1);
            }
            
            //Convert the time, save to log file and print to console
            time(&agentStartTime); //Record the time for the agent
            strcpy(printTime, ctime(&agentStartTime)); //Copy to the printTime array
            printTime[strlen(printTime) - 1] = '\0'; //Replace the new line
            fprintf(logFile, "%s: Responded to agent %s with invalid command\n", printTime, agentIP);
            printf ("%s: Responded to agent %s with invalid command\n", printTime, agentIP);

            
        }
        
    fclose(logFile); //Close the log file
	close(agentSocketFD);
    }
    
    return 0;
}
