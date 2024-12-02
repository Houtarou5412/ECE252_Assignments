#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>

#define PORT_1 "2527"
#define PORT_2 "2528"
#define PORT_3 "2529"
#define SERVER_IP "34.226.202.123"

char* ok_msg = "OK";
int checking_first = 1;

void listen_for_connections( int, int, int, char *recv_buff);


int main( int argc, char** argv ) {
    struct addrinfo hints;
    struct addrinfo *porting1;
    struct addrinfo *porting2;
    struct addrinfo *porting3;
    
    int id_socket[3]; 

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    char *rcv_buff = malloc(sizeof(char)*1000);
    memset(rcv_buff, 0, sizeof(char)*1000);

    getaddrinfo(SERVER_IP, PORT_1, &hints, &porting1);
    getaddrinfo(SERVER_IP, PORT_2, &hints, &porting2);
    getaddrinfo(SERVER_IP, PORT_3, &hints, &porting3);

    id_socket[0] = socket(porting1->ai_family,porting1->ai_socktype, porting1->ai_protocol);
    id_socket[1] = socket(porting2->ai_family,porting2->ai_socktype, porting2->ai_protocol);
    id_socket[2] = socket(porting3->ai_family,porting3->ai_socktype, porting3->ai_protocol);

    connect(id_socket[0],porting1->ai_addr,porting1->ai_addrlen);
    connect(id_socket[1],porting2->ai_addr,porting2->ai_addrlen);
    connect(id_socket[2],porting3->ai_addr,porting3->ai_addrlen);

    listen_for_connections(id_socket[0], id_socket[1], id_socket[2], rcv_buff);

    printf("Done!\n");
    for(int i = 0; i < 3; i++){
        close(id_socket[i]);
    }
    freeaddrinfo(porting1);
    freeaddrinfo(porting2);
    freeaddrinfo(porting3);
    free(rcv_buff);
    return 0;
}

/* This is the code from lecture 26 as your starter code; modify as needed */
void listen_for_connections( int service1_sock, int service2_sock, int service3_sock, char *recv_buff) {
    int nfds = 1 + (service1_sock > service2_sock
            ? service1_sock > service3_sock ? service1_sock : service3_sock
            : service2_sock > service3_sock ? service2_sock : service3_sock);

    fd_set s;
    struct timeval tv;
    bool quit = false;
    int nbytes = 0; 
    int nbytes2 = 0;
    int nbytes3 = 0;
    int first_time1 = 1;
    int first_time2 = 1;
    int first_time3 = 1;
    int input = 0;
    int input2 = 0;
    int input3 = 0;
    int quitting = 0;

    while( !quit ) {

        FD_ZERO( &s );
        FD_SET( service1_sock, &s );
        FD_SET( service2_sock, &s );
        FD_SET( service3_sock, &s );

        tv.tv_sec = 30;
        tv.tv_usec = 0;

        int res = select( nfds, &s, NULL, NULL, &tv );
        if ( res == -1 ) { /* An error occurred */
            printf( "An error occurred in select(): %s.\n", strerror( errno ) );
            quit = 1;
        } else if ( res == 0 ) { /* 0 sockets had events occur */
            printf( "Still waiting; nothing occurred recently.\n" );
        } else{ /* Things happened */
            if(first_time1 == 1){
                input = 4;
            }
            else{
                input = nbytes;
            }
            if(first_time2 == 1){
                input2 = 4;
            }
            else{
                input2 = nbytes2;
            }
            if(first_time3 == 1){
                input3 = 4;
            }
            else{
                input3 = nbytes3;
            }

            if ( FD_ISSET( service1_sock, &s ) ) { // sth happened in the socket
                recv(service1_sock, recv_buff, input, 0);
                if(first_time1 == 1){
                    memcpy(&nbytes, recv_buff, 4);
                    nbytes = ntohl(nbytes);
                    send(service1_sock, ok_msg, strlen(ok_msg),0);
                    first_time1 = 0;
                }
                else{
                    if(nbytes != 0){
                        printf("%s\n", recv_buff);
                        memset(recv_buff,0, sizeof(recv_buff));
                        quitting+=1;
                        nbytes = 0;
                    }
                }
            }
            if ( FD_ISSET( service2_sock, &s ) ) {
                recv(service2_sock, recv_buff, input2, 0);
                if(first_time2 == 1){
                    memcpy(&nbytes2, recv_buff, 4);
                    nbytes2 = ntohl(nbytes2);
                    send(service2_sock, ok_msg, strlen(ok_msg),0);
                    first_time2 = 0;
                }
                else{
                    if(nbytes2 != 0){
                        printf("%s\n", recv_buff);
                        memset(recv_buff,0, sizeof(recv_buff));
                        quitting+=1;
                        nbytes2 = 0;
                    }
                }
            }
            if ( FD_ISSET( service3_sock, &s ) ) {
                recv(service3_sock, recv_buff, input3, 0);
                if(first_time3 == 1){
                    memcpy(&nbytes3, recv_buff, 4);
                    nbytes3 = ntohl(nbytes3);
                    send(service3_sock, ok_msg, strlen(ok_msg),0);
                    first_time3 = 0;
                }
                else{
                    if(nbytes3 != 0){
                        printf("%s\n", recv_buff);
                        memset(recv_buff,0, sizeof(recv_buff));
                        quitting+=1;
                        nbytes3 = 0;
                    }
                }
            }
        }
        if(quitting == 3){
            break;
        }
    }
}