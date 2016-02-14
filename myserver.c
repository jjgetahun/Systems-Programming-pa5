#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <signal.h>

typedef struct Account{
    pthread_mutex_t accountLock;
    char name[100];
    float balance;
    int session;
} account;

typedef struct Bank{
    pthread_mutex_t bankLock;
    account accounts[20];
    int numAccounts;
} bank;

bank * myBank;

account * tempAccount = NULL;

int sockd;

pthread_mutexattr_t mutattrBank;
pthread_mutexattr_t mutattrAcct;
pthread_t bankInfo;

void sigchld_handler(int signo) {

    if (signo == SIGCHLD) {

        while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {

        }

    }

}

int openAccount(int sock_desc, char name []) {
    
    char message[500];
    
    int count = 1;

    while (pthread_mutex_trylock(&myBank->bankLock) != 0) {
        
        if (count > 8) {
            strcpy(message, "The bank is currently overloaded with clients. Please try again later.\n\n");
            write(sock_desc, message, sizeof(message)-1);
            strcpy(message, "Enter \"open [your name here]\" to open an account.\nEnter \"start [your name here]\" to start a session.\nEnter \"credit [your amount here]\" for credit.\nEnter \"debit [your amount here]\" for debit.\nEnter \"balance\" for your balance.\nEnter \"finish\" to finish a session.\nEnter \"exit\" to exit.\n");
            write(sock_desc, message, sizeof(message)-1);
            return 1;
        }

        sprintf(message, "The bank is busy due to another client search. Please hold...(%d)\n\n", count);

        write(sock_desc, message, sizeof(message)-1);

        count++;

        sleep(2);        

    }
    
    if (strlen(name) == 0) {
        strcpy(message, "You have not entered a name.\n\n");
        write(sock_desc, message, sizeof(message)-1);
        pthread_mutex_unlock(&myBank->bankLock);
        return 1;
    }

    if (myBank->numAccounts == 20) {        
        strcpy(message, "The maximum number of accounts has been reached. You may not open one at this time.\n\n");

        write(sock_desc, message, sizeof(message)-1);
        
        pthread_mutex_unlock(&myBank->bankLock);
        
        return 1;
    }
    
    else {

        int i;

        for (i = 0; i < myBank->numAccounts; i++) {
            if (strcmp(myBank->accounts[i].name, name) == 0) {
                
                strcpy(message, "There exists an account with the name that you have provided. You may not open one at this time.\n\n");

                write(sock_desc, message, sizeof(message)-1);

                pthread_mutex_unlock(&myBank->bankLock);

                return 1;
            }
        }       
        
        strcpy(myBank->accounts[i].name, name);
        myBank->accounts[i].balance = 0;
        myBank->accounts[i].session = 0;
        pthread_mutex_init(&myBank->accounts[myBank->numAccounts].accountLock, &mutattrAcct);
        myBank->numAccounts++;

        sprintf(message, "Thank you for opening an account with us, %s!\n\n", name);
        write(sock_desc, message, sizeof(message)-1);

        pthread_mutex_unlock(&myBank->bankLock);

        return 0;       
    }
}

account * startAccount(int sock_desc, char name []) {

    char message[500];

    int count = 1;       

    while (pthread_mutex_trylock(&myBank->bankLock) != 0) {
        
        if (count > 8) {
            strcpy(message, "The bank is currently busy. Please try again later.\n\n");
            write(sock_desc, message, sizeof(message)-1);
            strcpy(message, "Enter \"open [your name here]\" to open an account.\nEnter \"start [your name here]\" to start a session.\nEnter \"credit [your amount here]\" for credit.\nEnter \"debit [your amount here]\" for debit.\nEnter \"balance\" for your balance.\nEnter \"finish\" to finish a session.\nEnter \"exit\" to exit.\n");
            write(sock_desc, message, sizeof(message)-1);
            return NULL;
        }
       
        sprintf(message, "The bank is currently locked due to another client search. Please hold...(%d)\n\n", count);

        write(sock_desc, message, sizeof(message)-1);
    
        count++;

        sleep(2);

    }

    if (strlen(name) == 0) {
        strcpy(message, "You have not entered a name.\n\n");
        write(sock_desc, message, sizeof(message)-1);
        pthread_mutex_unlock(&myBank->bankLock);
        return NULL;
    }
    
    int i;

    for (i = 0; i < myBank->numAccounts; i++) {

        if (strcmp(myBank->accounts[i].name, name) == 0) {
            break;
        }

    }

    //pthread_mutex_unlock(&myBank->bankLock);

    if (myBank->numAccounts == i) {

        sprintf(message, "There are no accounts with the name \"%s\" currently open. Please try again later.\n\n", name);

        write(sock_desc, message, sizeof(message)-1);

        pthread_mutex_unlock(&myBank->bankLock);
       
        return NULL;

    }

    else {

        count = 1;

        while (pthread_mutex_trylock(&myBank->accounts[i].accountLock) != 0) {
        
            if (count > 8) {
                sprintf(message, "%s is currently in session. Please try again later.\n\n", name);
                write(sock_desc, message, sizeof(message)-1);
                strcpy(message, "Enter \"open [your name here]\" to open an account.\nEnter \"start [your name here]\" to start a session.\nEnter \"credit [your amount here]\" for credit.\nEnter \"debit [your amount here]\" for debit.\nEnter \"balance\" for your balance.\nEnter \"finish\" to finish a session.\nEnter \"exit\" to exit.\n");
                write(sock_desc, message, sizeof(message)-1);
                pthread_mutex_unlock(&myBank->bankLock);
                return NULL;
            }      
        
            sprintf(message, "%s's account is currently in session. Please hold...(%d)\n\n", name, count);

            write(sock_desc, message, sizeof(message)-1);
        
            count++;

            sleep(2);

    }

        myBank->accounts[i].session = 1;

        strcpy(message, "You have successfully started a session.\n\n");
        write(sock_desc, message, sizeof(message)-1);
        pthread_mutex_unlock(&myBank->bankLock);
        return &myBank->accounts[i];
    }

}

void credit(int sock_desc, account * acc, float val) {
    char message[500];
    
    acc->balance += val;
    sprintf(message, "You have successfully credited $%.2f to your account.\n\n", val);
    write(sock_desc, message, sizeof(message)-1);
}


void debit(int sock_desc, account * acc, float val) {
    
    if (acc->balance < val) {
        char message[500];
        strcpy(message, "The amount that you have entered is greater than your balance. You may not debit from your account at this time.\n\n");
        write(sock_desc, message, sizeof(message)-1);
    }

    else {
        char message[500];
        acc->balance -= val;
        sprintf(message, "You have successfully debited $%.2f from your account.\n\n", val);
        write(sock_desc, message, sizeof(message)-1);
    }
}

void balance(int sock_desc, account * acc) {
    char message[500];
    sprintf(message, "Your account balance is $%.2f.\n\n", acc->balance);
    write(sock_desc, message, sizeof(message)-1);
}

void finishAccount(int sock_desc, account * acc) {
    char message[500];
    acc->session = 0;
    strcpy(message, "You have successfully finished your session.\n\n");
    write(sock_desc, message, sizeof(message)-1);
    pthread_mutex_unlock(&acc->accountLock);
}

void exitSession(int sock_desc, account * acc) {
    char message[500];
    acc->session = 0;
    strcpy(message, "You have exited the bank.\n\n");
    write(sock_desc, message, sizeof(message)-1);
    printf("A client has disconnected from the server.\n");
    pthread_mutex_unlock(&acc->accountLock);
}

void client_service(int * sock_desc) {

    sockd = *(int *)sock_desc;

    char buffer[500];

    char response[500];

    while (read(sockd, buffer, sizeof(buffer)-1) > 0) {
        char command[500];
        char nameOrVal[100];
        
        sscanf(buffer, "%s %[^\n]", command, nameOrVal);

        if (strcmp(command, "open") == 0) {
           
            if (tempAccount!= NULL && tempAccount->session != 0) {

                strcpy(response, "You are already logged in. You may not open an account.\n\n");

                write(sockd, response, sizeof(response)-1);

            }

            else {
                
                openAccount(sockd, nameOrVal);

            }
            
        }
        
        else if (strcmp(command, "start") == 0) {

            if (tempAccount != NULL && tempAccount->session != 0) {

                strcpy(response, "You are already logged in. You may not start an account.\n\n");

                write(sockd, response, sizeof(response)-1);

            }

            else {

                tempAccount = startAccount(sockd, nameOrVal);

            }

        }

        else if (strcmp(command, "credit") == 0) {

            if (strlen(nameOrVal) == 0) {
                strcpy(response, "You have not entered a value.\n\n");
                write(sockd, response, sizeof(response)-1);
                continue;
            }

            if (tempAccount == NULL || tempAccount->session == 0) {

                strcpy(response, "You are not logged in. You may not add to credit at this time.\n\n");

                write(sockd, response, sizeof(response)-1);

            }

            else {

                float val = atof(nameOrVal);

                float round = roundf(val*100)/100;

                if ((fabs(round - 0.0f) < 0.00001) || (round < 0.0f)) {
                    
                    strcpy(response, "The value that you have entered is 0 or invalid.\n\n");

                    write(sockd, response, sizeof(response)-1);

                }

                else {

                    credit(sockd, tempAccount, round);

                }

            }

        }

        else if (strcmp(command, "debit") == 0) {

            if (strlen(nameOrVal) == 0) {
                strcpy(response, "You have not entered a value.\n\n");
                write(sockd, response, sizeof(response)-1);
                continue;
            }

            if (tempAccount == NULL || tempAccount->session == 0) {

                strcpy(response, "You are not logged in. You may not subtract from debit at this time.\n\n");

                write(sockd, response, sizeof(response)-1);

            }

            else {
                
                float val = atof(nameOrVal);

                float round = roundf(val*100)/100;

                if ((fabs(round - 0.0f) < 0.00001) || (round < 0.0f)) {

                    strcpy(response, "The value that you have entered is 0 or invalid.\n\n");

                    write(sockd, response, sizeof(response)-1);

                }

                else {

                    debit(sockd, tempAccount, round);

                }

            }

        }

        else if (strcmp(command, "balance") == 0) {

            if (tempAccount == NULL || tempAccount->session == 0) {

                strcpy(response, "You are not logged in. You do not have access to an account balance at this time.\n\n");

                write(sockd, response, sizeof(response)-1);

            }

            else if (*nameOrVal != '\0') {
                
                strcpy(response, "The command that you have entered is not valid. Please try again.\n\n");

                write(sockd, response, sizeof(response)-1);

            }

            else {

                balance(sockd, tempAccount);

            }

        }

        else if (strcmp(command, "finish") == 0) {

            if (tempAccount == NULL || tempAccount->session == 0) {

                strcpy(response, "You are not logged in. You cannot finish a session at this time.\n\n");

                write(sockd, response, sizeof(response)-1);

            }

            else if (*nameOrVal != '\0') {
                
                strcpy(response, "The command that you have entered is not valid. Please try again.\n\n");

                write(sockd, response, sizeof(response)-1);

            }

            else {

                finishAccount(sockd, tempAccount);

            }

        }

        else if (strcmp(command, "exit") == 0) {
            
            if (tempAccount == NULL) {

                strcpy(response, "You have exited the bank. Have a great day!\n\n");
                write(sockd, response, sizeof(response)-1);

                printf("A client has disconnected from the server.\n");

                close(sockd);

                break;

            }

            else if (*nameOrVal != '\0') {
                
                strcpy(response, "The command that you have entered is not valid. Please try again.\n\n");

                write(sockd, response, sizeof(response)-1);

            }

            else {

                exitSession(sockd, tempAccount);

                close(sockd);

                break;

            }

        }

        else {

            strcpy(response, "The command that you have entered is not valid. Please try again.\n\n");

            write(sockd, response, sizeof(response)-1);

        }

        memset(command, '\0', sizeof(command));
        memset(nameOrVal, '\0', sizeof(nameOrVal));
        memset(buffer, '\0', sizeof(buffer));
        memset(response, '\0', sizeof(response));

    }

}

void printBankInfo(void * ptr) {

    int sock_desc = *((int *) ptr);

    int count = 1;

    while (pthread_mutex_trylock(&myBank->bankLock) != 0) {
        
        char message[500];

        if (count > 8) {
            strcpy(message, "The bank is currently overloaded with clients. Please try again later.\n\n");
            write(sock_desc, message, sizeof(message)-1);
            strcpy(message, "Enter \"open [your name here]\" to open an account.\nEnter \"start [your name here]\" to start a session.\nEnter \"credit [your amount here]\" for credit.\nEnter \"debit [your amount here]\" for debit.\nEnter \"balance\" for your balance.\nEnter \"finish\" to finish a session.\nEnter \"exit\" to exit.\n");
            write(sock_desc, message, sizeof(message)-1);
            return;
        }
        
        sprintf(message, "The bank is busy processing client information. Please hold...(%d)\n\n", count);

        count++;

        sleep(2);

    }

    while(1){

        int i;

        for (i = 0; i < myBank->numAccounts; i++) {
            printf("Account name: %s\n", myBank->accounts[i].name);
            printf("Balance: $%.2f\n", myBank->accounts[i].balance);
            if (myBank->accounts[i].session == 1)
                printf("IN SERVICE\n");
            else
                ;
            printf("\n");
        }

        pthread_mutex_unlock(&myBank->bankLock);

        sleep(20);
    }

}

int main (int argc, char ** argv) {

    char buffer[500];


    struct sockaddr_in saddr;

    socklen_t saddrlen;

    int check, sd;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Socket error.\n");
        exit(1);
    }

    int portnum = 6969;

    bzero((char *)&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(portnum);
    saddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        printf("%s\n", strerror(errno));
        fprintf(stderr, "socket--bind error.\n");
        exit(1);
    }

        /*---Make it a "listening socket"---*/
    if (listen(sd, 20) < 0) {
        fprintf(stderr, "socket--listen error.\n");
        exit(1);
    }

    key_t key;
    int	shmid;
    int	size = 4096;
    
    if (errno = 0, (key = ftok( ".", 42 )) == -1){
        printf("ftok() failed  errno :  %s\n", strerror(errno));
        exit(1);
    }
    
    else if (errno = 0, (shmid = shmget( key, size, 0666 | IPC_CREAT | IPC_EXCL )) != -1) {		// create ok?{
        
        errno = 0;
        myBank = (bank *)shmat(shmid, 0, 0);
        pthread_mutexattr_init(&mutattrBank);
        pthread_mutexattr_setpshared(&mutattrBank, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&myBank->bankLock, &mutattrBank);
        //if ( p == (void *)-1 ){
        if (myBank == (void *)-1){
            printf( "shmat() failed  errno :  %s\n", strerror(errno));
            exit(1);
        }
        else{

            myBank->numAccounts = 0;

            int clientsd;
            
            while((clientsd = accept(sd, (struct sockaddr *)&saddr, &saddrlen)) != -1){
                
                pthread_create(&bankInfo, NULL, (void *) printBankInfo, (void *) &sd);


                int pid = fork();

                if (pid != 0) { /*If this is the parent process*/
                   
                    signal(SIGCHLD, sigchld_handler);
                    shmctl(shmid, IPC_RMID, NULL); /*Removes the shared memory segment*/
                    close(clientsd); /*Closes the socket descriptor*/

                }

                else {

                    int * sd = (int *)malloc(sizeof(int));

                    *sd = clientsd;

                    printf("Connection from a client has been accepted.\n");

                    client_service(sd);

                }

            }

        }

    
    }
    
    else if (errno = 0, (shmid = shmget( key, 0, 0666 )) != -1 ){					// find ok?{
        errno = 0;
        
        myBank = (bank *)shmat(shmid, 0, 0);
        pthread_mutexattr_init(&mutattrBank);
        pthread_mutexattr_setpshared(&mutattrBank, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&myBank->bankLock, &mutattrBank);
        
        pthread_mutexattr_init(&mutattrAcct);
        pthread_mutexattr_setpshared(&mutattrAcct, PTHREAD_PROCESS_SHARED);
        if (myBank == (void *)-1){
            printf( "shmat() failed  errno :  %s\n", strerror(errno));
            exit(1);
        }
        else{
            
            int clientsd;
            
            while((clientsd = accept(sd, (struct sockaddr *)&saddr, &saddrlen)) != -1){

                pthread_create(&bankInfo, NULL, (void *) printBankInfo, (void *) &sd);

                int pid = fork();

                if (pid != 0) { /*If this is the parent process*/

                    signal(SIGCHLD, sigchld_handler);
                    shmctl(shmid, IPC_RMID, NULL); /*Removes the shared memory segment*/
                    close(clientsd); /*Closes the socket descriptor*/

                }

                else {

                    int * sd = (int *)malloc(sizeof(int));

                    *sd = clientsd;

                    printf("Connection from a client has been accepted.\n");

                    client_service(sd);

                }

            }

        }

    }

    else{											// no create, no find{
        printf( "shmget() failed  errno :  %s\n", strerror( errno ) );
        exit(1);
    }

    return 0;

}
