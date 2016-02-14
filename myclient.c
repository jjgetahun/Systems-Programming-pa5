#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

int sd;

static void signal_handler(int signo){

    if (signo == SIGINT) {

        char message[100] = "exit";
        
        write(sd, message, sizeof(message)-1);

        printf("You have disconnected from the server. Connection closed.\n"); 

        exit(0);

    }

}

void command_input(void * ptr) {

    char buffer[500];

    int sock_desc = *((int *) ptr);

    int check;

    while((check = read(sock_desc, buffer, sizeof(buffer)-1)) > 0) {
        printf("%s", buffer);
        memset(buffer, '\0', sizeof(buffer));
    }

    if (check == 0) {
        printf("Cannot read from the server. Connection closed.\n");
        exit(0);
    }

}

void response_output(void * ptr) {

    char buffer[500];

    int sock_desc = *((int *) ptr);

    int check;

    printf("Enter \"open [your name here]\" to open an account.\nEnter \"start [your name here]\" to start a session.\nEnter \"credit [your amount here]\" for credit.\nEnter \"debit [your amount here]\" for debit.\nEnter \"balance\" for your balance.\nEnter \"finish\" to finish a session.\nEnter \"exit\" to exit.\n");

        while((check = read(0, buffer, sizeof(buffer)-1)) > 0) {
                 
            int n = write(sock_desc, buffer, sizeof(buffer)-1);
            if(n ==0){
                printf("Cannot write to server. Connection closed.\n");
                exit(0);
            }
            sleep(2);
            
            printf("Enter \"open [your name here]\" to open an account.\nEnter \"start [your name here]\" to start a session.\nEnter \"credit [your amount here]\" for credit.\nEnter \"debit [your amount here]\" for debit.\nEnter \"balance\" for your balance.\nEnter \"finish\" to finish a session.\nEnter \"exit\" to exit.\n");
            
            memset(buffer, '\0', sizeof(buffer));
        }

}

int main(int argc, char *argv[]) {
    struct addrinfo request;
    struct addrinfo *result, *rp;
    int s, j;
    size_t len;
    ssize_t nread;

    struct sockaddr_in dest;
    struct hostent * server;
    socklen_t saddrlen;

    struct sigaction action;

    action.sa_flags = 0;
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, 0);

    if (argc < 2) {
        fprintf(stderr, "Usage: %s host\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Socket error.\n");
        exit(1);
    }

    server = gethostbyname(argv[1]);
    
    if (server == NULL) {
        fprintf(stderr, "The host that you have given does not exist.\n");
        exit(1);
    }

    int portnum = 6969;

    bzero((char *)&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&dest.sin_addr.s_addr, server->h_length);
    dest.sin_port = htons(portnum);
    
        /*---Connect to server---*/
    if (connect(sd, (struct sockaddr*)&dest, sizeof(dest)) != 0 ) {
        printf("%s\n", strerror(errno));
        fprintf(stderr, "Connection error.\n");
        close(sd);
        exit(1);
    }

    printf("Connection to server has been established.\n");

    pthread_t command, response;

    pthread_create(&command, NULL, (void *) command_input, (void *) &sd);
    pthread_create(&response, NULL, (void *) response_output, (void *) &sd);

    pthread_join(command, NULL);
    pthread_join(response, NULL);
  
    exit(0);
}
