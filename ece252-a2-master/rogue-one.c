#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT "8600"
#define PLANS_FILE "deathstarplans.dat"

typedef struct {
    char * data;
    int length;
} buffer;

extern int errno;

/* This function loads the file of the Death Star
   plans so that they can be transmitted to the
   awaiting Rebel Fleet. It takes no arguments, but
   returns a buffer structure with the data. It is the
   responsibility of the caller to deallocate the 
   data element inside that structure.
   */ 

buffer load_plans( );

int sendall(int socket, char *buf, int *len);
//port is 80
//ip is arcv
int main( int argc, char** argv ) {

    if ( argc != 2 ) {
        printf( "Usage: %s IP-Address\n", argv[0] );
        return -1;
    }
    printf("Planning to connect to %s.\n", argv[1]);

    buffer buf = load_plans();

    struct addrinfo hints; 
    struct addrinfo *res; 
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int result = getaddrinfo(argv[1], PORT, &hints, &res);
    int clientsock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    int connection = connect(clientsock, res->ai_addr, res->ai_addrlen);
    //printf("Connection value: %d, errno: %d\n", connection, errno);
    if(connection == -1){
        perror("Error printed by perror");
    }

    
    sendall(clientsock, buf.data, &buf.length); 
    char data[64];
    int receiving = recv(clientsock, data, 64, 0);
    if(receiving == -1){
        printf("Receiving failed error number: %d\n", errno);
        perror("Error in text is");
    }
    else{
        printf("%s", data);
    }

    close(clientsock);
    freeaddrinfo(res);
    free(buf.data);
    return 0;
}

buffer load_plans( ) {
    struct stat st;
    stat( PLANS_FILE, &st );
    ssize_t filesize = st.st_size;
    char* plansdata = malloc( filesize );
    int fd = open( PLANS_FILE, O_RDONLY );
    memset( plansdata, 0, filesize );
    read( fd, plansdata, filesize );
    close( fd );

    buffer buf;
    buf.data = plansdata;
    buf.length = filesize;

    return buf;
}

int sendall(int socket, char *buf, int*len){
    int total = 0; // This is how many bytes that were sent
    int bytesleft = *len; // How many we have left that needs to be sent
    int n;

    while (total < *len){
        n = send(socket, buf + total, bytesleft, 0);
        if(n == -1){
            printf("The error number is: %d\n", errno);
            perror("The error is");
            break;
        }
        total += n;
        bytesleft -= n;
    }
    *len = total; //return numbers that were actually sent
    return n == -1 ? -1 : 0; // return -1 when it failed, if 0 then it is success
}
