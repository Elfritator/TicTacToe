/*
*   Protocol: [ message code ] [ 9 characters for grid state ]
*
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h> // for random
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MYPORT 5555
#define BACKLOG 0 //no limit in connections
#define DATASIZE 100
#define LENGTH 9

void sigchld_handler(int s){
    while(wait(NULL) > 0);
}

int main(void){
    
	int sockfd, new_fd; //listening socket and new connection socket
	struct sockaddr_in my_addr;    // my address
    struct sockaddr_in their_addr; // connector's address information
	socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;

    // initialization of listening socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // setsockopt to not wait for second bind
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    // creation of sockaddr_in struct for server
    my_addr.sin_family = AF_INET;         // host byte order
    my_addr.sin_port = htons(MYPORT);     // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

    // bind socket to port
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    // listen par le socket bindé
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    // clean-up of dead child processes
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    while(1){ // main accept loop, when accept -> fork
        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
            perror("accept");
            continue;
        }
        printf("server: got connection from %s\n",inet_ntoa(their_addr.sin_addr));
        printf("test1\n");
        
        if (!fork()) { // this is the child process for each client
            close(sockfd);
            short int choice;
            char buffer[DATASIZE];

            if (send(new_fd, "Envoyer le chiffre 1 pour jouer, le chiffre 2 pour se déconnecter\n", 67, 0) == -1)
                perror("send");
            
            // info recue dans variable buf
            if ((recv(new_fd, buffer, 1, 0)) == -1) {
                 perror("recv");
            }

            if(buffer[0] == '1'){ //si choisit jouer

                char grid[LENGTH]; //grid for game, if O -> server, X -> user, 0 -> empty
                int finish = 0;

                for (int i = 0; i < LENGTH; ++i)
                    grid[i] = ' ';
                char choice = '0';

                srand(time(NULL)); 
                int random = rand() % LENGTH; // random
                grid[random] = 'X';

                if (send(new_fd, "0", 1, 0) == -1) perror("send");
                if (send(new_fd, grid, LENGTH, 0) == -1) perror("send");
                if (send(new_fd, "La partie commence:\n", 20, 0) == -1) perror("send");
                printf("test\n");
                
                char ending[1] = "0";
                // boucle du jeu
                while (ending == "0"){

                    printf("testbuff\n");
                    if ((recv(new_fd, buffer, 1, 0)) == -1) {
                        perror("recv");
                    }

                    // il saute tout?
                    choice = buffer[0]-'0';
                    printf("choice: %i\n",choice);
                    
                    grid[choice] = 'O';
                    // verifier grille

                    //srand(time(NULL)); 
                    random = rand() % LENGTH; // random

                    while (grid[random] != ' '){
                        random = rand() % LENGTH;
                    }
                    grid[random] = 'X';
                    // vérifier grille

                    if (send(new_fd, ending, 1, 0) == -1){
                        perror("send");
                        exit(1);
                    }

                    if (send(new_fd, grid, LENGTH, 0) == -1) {
                        perror("send");
                        exit(1);
                    }
                    
                    if (ending == "0"){
                        if (send(new_fd, "La partie continue:\n", 20, 0) == -1){
                            perror("send");
                            exit(1);
                        }
                    }
               
                }

                if (ending == "1"){
                    if (send(new_fd, "La partie continue:\n", 20, 0) == -1){
                        perror("send");
                        exit(1);
                }
            
            }
            exit(0);
        }

            close(new_fd);  // parent doesn't need this
        }

    return 0;

    }
}