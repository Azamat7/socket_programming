/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include "utils.h"

#define BACKLOG 10   // how many pending connections queue will hold
#define MAXDATASIZE 10 * 1000 * 1000 - 8// max number of bytes we can get at once 

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    return &(((struct sockaddr_in*)sa)->sin_addr);
}

// $ ./server -p 1234

int main(int argc, char *argv[])
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    char *port;
    char *header = malloc(8);
    char *buffer = malloc(MAXDATASIZE); 

    if (argc != 3) { 
        printf("Usage: ./server -p PORT_NUMBER\n");
        return 1;
    }

    if (strcmp( argv[1], "-p") == 0) {
        port = argv[2];
    } else {
        printf("Please provide port number in the correct format: ./server -p PORT_NUMBER\n");
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        printf("server: got connection from the client\n");

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener

            int valread, header_length;
            header_length = read(new_fd , header, 8);

            uint16_t checksum = ((uint16_t *)header)[1];
            printf("\nserver checsum: %d\n", checksum);

            uint32_t length = ntohl(((uint32_t *)header)[1]);
            printf("\nlength: %d\n", length);

            // for (int i=0; i<8; i++){
            //     printf("%d", ntohs(header[i]));
            // }

            int operation = header[0];
            int shift = header[1];

            valread = read(new_fd , buffer, MAXDATASIZE); 
            for (int i=0; i<valread; i++){
                printf("%c", buffer[i]);
            }

            int16_t *hp = (int16_t *)header;
            ++hp; *hp = 0;
            uint16_t checksumValid = checksum2(header, (char *)buffer, length - 8);
            printf("\nvalid checksum: %d\n", checksumValid);

            cipher(buffer, operation, shift, valread);
            for (int i=0; i<valread; i++){
                printf("%c", buffer[i]);
            }
            uint16_t new_checksum = checksum2((char *)header, buffer, valread);
            printf("\nnew checksum: %d\n", new_checksum);
            uint16_t *bp = (uint16_t *)header; ++bp;
            *bp = new_checksum; ++bp;
            char * message = malloc(8 + valread);
            for (int i=0; i<8; i++){
                message[i] = header[i];
            }
            for (int i=8; i<(valread+8); i++){
                message[i] = buffer[i-8];
            }
            

            if (send(new_fd, message, valread+8, 0) == -1)
                perror("send");

            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}