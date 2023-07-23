#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MY_PORT_NUMBER 23221
#define MY_PORT_NUMBER_STRING_VER "23221"

int serverSide(int argc, char const *argv[]) {

    // Start by making a socket (SOCK_STREAM is TCP)
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1) {
        printf("%s\n", strerror(errno));
        fflush(stderr);
        exit(1);
    }

    // Clear the socket and make it available if the server is killed
    int setSocket = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if(setSocket == -1) {
        printf("%s\n", strerror(errno));
        fflush(stderr);
        exit(1);
    }

    //Bind the socket to a port
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(MY_PORT_NUMBER);
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(listenfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        perror("bind");
        fflush(stderr);
        exit(1);
    }

    //Set up a connection queue 10 levels deep
    int setListen = listen(listenfd, 10);
    if(setListen == -1) {
        printf("%s\n", strerror(errno));
        fflush(stderr);
        exit(1);
    }

    //Used for accepting connections
    int pid;
    int connectfd;
    int writing;
    int timesConnected = 0;
    int length = sizeof(struct sockaddr_in);
    struct sockaddr_in clientAddr;

    //Used for getting text host name
    char hostName[NI_MAXHOST];
    int hostEntry;

    //Infinite loop (server runs forever)
    for(;;) {

        //Accept connections
        connectfd = accept(listenfd, (struct sockaddr *) &clientAddr, &length);
        if(connectfd < 0) {
            printf("%s\n", strerror(errno));
            fflush(stderr);
            exit(1);
        }
        else {
            //Get text host name
            hostEntry = getnameinfo((struct sockaddr*)&clientAddr, sizeof(clientAddr),
                                    hostName, sizeof(hostName), NULL, 0, NI_NUMERICSERV);
            if(hostEntry != 0) {
                printf("Error: %s\n", gai_strerror(hostEntry));
                fflush(stderr);
                exit(1);
            }
            timesConnected++;
            printf("%s %d\n", hostName, timesConnected);
            fflush(stdout);

            //If connection accepted, create child process
            if((pid = fork()) == 0) {
                //Get time
                char currTime[19]; //Must be exactly 18 bytes(19 with "\n")
                
                time_t n;
                time(&n);
                strcpy(currTime, ctime(&n));
                currTime[18] = '\n';

                //Write to stdout
                writing = write(connectfd, currTime, 19);
                if(writing == -1) {
                    printf("%s\n", strerror(errno));
                    fflush(stderr);
                    exit(1);
                }
                exit(0); //Succesful execution
            }
            //Close off parent and wait for child
            else {
                int status;
                close(connectfd);
                waitpid(-1, &status, WNOHANG);
            }
        }
    }
}


int clientSide(int argc, char const *argv[]) {

    //Set up the address of the server
    struct addrinfo hints, *actualdata;
    memset(&hints, 0, sizeof(hints));

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    //Setup address of the server
    int err = getaddrinfo(argv[2], MY_PORT_NUMBER_STRING_VER, &hints, &actualdata);
    if(err != 0) {
        printf("Error: %s\n", gai_strerror(err));
        fflush(stderr);
        exit(1);
    }

    int socketfd = socket(actualdata->ai_family, actualdata->ai_socktype, 0);
    if(socketfd == -1) {
        printf("%s\n", strerror(errno));
        fflush(stderr);
        exit(1);
    }

    //Connect to the server
    int attemptConnect = connect(socketfd, actualdata->ai_addr, actualdata->ai_addrlen);
    if(attemptConnect != 0) {
        printf("%s\n", strerror(errno));
        fflush(stderr);
        exit(1);
    }

    //Read from server
    char buff[19];
    int reading = read(socketfd, buff, 19);
    if(reading == -1) {
        printf("%s\n", strerror(errno));
        fflush(stderr);
        exit(1);
    }
    else {
        write(1, buff, reading); 
        fflush(stdout);
        close(socketfd);
    }
}


int main(int argc, char const *argv[]){

    //Server argument specified 
    if(strcmp(argv[1], "server") == 0) {
        if(argc != 2) { 
            printf("%s\n", strerror(errno));
            exit(1);
        }
        serverSide(argc, argv);
    }
    //Client argument specified
    else if(strcmp(argv[1], "client") == 0) {
        if(argc != 3) {
            printf("%s\n", strerror(errno));
            exit(1);
        }
        clientSide(argc, argv);
    }
    //Incorrect argument(s)
    else {
        printf("Incorrect arg(s) or arg count.\n");
        exit(1);
    }    

    return 0;
}
