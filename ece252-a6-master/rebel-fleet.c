#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <errno.h>
#include <sys/time.h>



#define PORT 8600


volatile int quit = 0;
char* plansfile = "deathstarplans.dat";
char* plansbuf;
ssize_t filesize;

char* msg_ok = "Success - Death Star Plans Received!\n";
char* err_inc = "Error - Incomplete Data Received.\n";
char* err_long = "Error - More Data Than Expected Received.\n";
char* err_corr = "Error - Data corrupted.\n";

void signal_handler(int dummy_variable) {
    quit = 1;
}


int main(int argc, char** argv) {
    struct stat st;
    stat(plansfile, &st);
    filesize = st.st_size;
    plansbuf = malloc(filesize);
    memset(plansbuf, 0, filesize);
    int fd = open(plansfile, O_RDONLY);
    read(fd, plansbuf, filesize);
    int timeout = 30 * 1000; // delay of 30 seconds

    /* Create Socket, IPv4 / Stream */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    //printf("Socket Created \n");
    /* Create the sockaddr in on port */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sockfd, 15); // usually limtied to 20 so I believed 15 is safe
    struct pollfd pollfds[16];
    memset(&pollfds, 0, sizeof(struct pollfd) * 16);
    char* buff_received = malloc(filesize + 100000);

    pollfds[0].fd = sockfd;
    pollfds[0].events = POLLIN;
    nfds_t fds_number = 1;
    int new_fd_encounter = 0;
    int c = 0; 
    signal(SIGINT, signal_handler);
    //printf("Signal is running \n");
    //printf("Going to start listening for socket events.\n");
    while (!quit) {
        //printf("quit is not equal to 1\n");
        int res = poll(pollfds, fds_number, timeout);
        //printf("res has been made \n");
        if (res == -1) { /* An error occurred */
            quit = 1;
        }
        else if (res == 0) { /* 0 sockets had events occur */
            printf("Still listening...\n");

        }
        else { /* Things happened */
            //printf("Things Happened\n");          
            if (pollfds[0].revents & POLLIN) {
                while (1) {
                    new_fd_encounter = accept(sockfd, NULL, NULL);
                    if (new_fd_encounter == 0 || new_fd_encounter < 0) {
                        break;
                    }
                    else{
                        struct timeval tv;
                        tv.tv_sec = 0;
                        tv.tv_usec = 100000;
                        setsockopt(new_fd_encounter, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                        pollfds[fds_number].fd = new_fd_encounter;
                        pollfds[fds_number].events = POLLIN;
                        fds_number += 1;
                    }
                }
            }
            
            
            //printf("before it enters c < new_fd_encounter\n");
            //printf("value of new_fd_encounter: %d\n", new_fd_encounter);
            for(int i = 1; i < fds_number; i++) {
                if (pollfds[i].revents & POLLIN) {
                    int recv_bytes = 0;
                    int recvs = 0;
                    

                    recvs = recv(pollfds[i].fd, buff_received, 100000, 0);
                    while (recvs > 0) {
                        if(recv_bytes > filesize){
                            break;
                        }
                        recv_bytes += recvs;
                        recvs = recv(pollfds[i].fd, buff_received + recv_bytes, 100000, 0);
                    }

                    if (recv_bytes > filesize) {
                        send(pollfds[i].fd, err_long, strlen(err_long), 0);
                    }

                    else if (recv_bytes < filesize) {
                        send(pollfds[i].fd, err_inc, strlen(err_inc), 0);
                    }

                    
                    else {
                        if (!memcmp(buff_received, plansbuf, filesize)) {
                            send(pollfds[i].fd, msg_ok, strlen(msg_ok), 0);
                        }
                        else{
                            send(pollfds[i].fd, err_corr, strlen(err_corr), 0);
                        }
                    }

                    close(pollfds[i].fd);
                    pollfds[i].fd = -1;
                    pollfds[i].revents = 0;

                    int finished_closing = 0;
                    for(int j = 1; j < fds_number; j++) {
                        if (pollfds[j].fd == -1) {
                            finished_closing += 1;
                        }
                    }

                    if (finished_closing == fds_number -1) {
                        fds_number = 1;
                    }
                }
            }
        }
    }
    close(pollfds[0].fd);
    free(buff_received);
    free(plansbuf);
    close(fd);
    return 0;
}

