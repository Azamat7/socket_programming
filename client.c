/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>

#include <arpa/inet.h>

#include "utils.h"

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 10 * 1000 * 1000// max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    return &(((struct sockaddr_in*)sa)->sin_addr);
}

// -h 143.248.111.222 -p 1234 -o 0 -s 5
int main(int argc, char *argv[])
{
    int sockfd, numbytes;  
    char *output = malloc(MAXDATASIZE-8);
    char *buf = malloc(MAXDATASIZE-8);
    char *header = malloc(8);
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    char *port;
    int operation;
    int shift;
    char *host;

    if (argc != 9) { 
        printf("example usage: ./client -h 143.248.111.222 -p 1234 -o 0 -s 5\n");
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        switch (argv[i][1]){
            case 'p':
                port = argv[++i];
                break;
            case 'o':
                operation = atoi(argv[++i]);
                break;
            case 's':
                shift = (atoi(argv[++i]) % 26 + 26) % 26;
                break;
            case 'h':
                host = argv[++i];
                break;
            default:
                printf("Please provide correct arguments: -h, -p, -o, -s\n");
                return 1;
        }
    }

    uint16_t *message = malloc(MAXDATASIZE);
    uint16_t *mp = message;
    //
    *mp = htons(256 * operation + shift); ++mp;
    *mp = htons(0); ++mp;
    *mp = htons(0); ++mp; //length
    *mp = htons(0); ++mp; //length

    uint16_t *buffer = malloc(1000);

    int n;
    uint32_t length = 8;
    do {
        n = read(0, buffer, 1000);

        if ((length+n) >= MAXDATASIZE){
            printf("overflowed");
            return -1;
        }

        length += n;

        for (int i = 0; i < n; i++) {
		    *mp = buffer[i]; ++mp;
	    }

    } while(n > 0);

    uint16_t ua, ub;
    split_length(length, &ua, &ub);
    uint16_t *mpl = message;
    ++mpl;++mpl;
    *mpl = htons(ua); ++mpl; //length
    *mpl = htons(ub); ++mpl; //length

    uint16_t *mplc = message;
    ++mplc;
    uint16_t checksum = checksum1((char *) message, length);
    printf("\nchecksum: %d\n", checksum);
    *mplc = checksum; ++mpl;
    

    for (int i=0; i<504; i++){
        printf("%d", message[i]);
    }
    // printf("%lu\n\n", strlen(message));

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    printf("client: connecting to server \n");

    freeaddrinfo(servinfo); // all done with this structure
    
    send(sockfd, message, length, 0);


    char * header_buffer = malloc(8);
    int received_bytes;
    int header_bytes = 0;
    int output_bytes = 0;
    while (header_bytes < 8){
        received_bytes = recv(sockfd, header_buffer, 8, 0);
        if ((header_bytes + received_bytes) >= 8){
            for (int i=header_bytes; i<8; i++){
                header[i] = header_buffer[i-header_bytes];
            }
            for (int i=0; i<(received_bytes+header_bytes-8); i++){
                output[i] = header_buffer[8-header_bytes+i];
            }
            output_bytes = received_bytes-8+header_bytes;
            header_bytes = 8;
        }else{
            for (int i=0; i<received_bytes; i++){
                header[header_bytes + i] = header_buffer[i];
            }
            header_bytes += received_bytes;
        }
    }

    
    
    uint16_t server_checksum = ((uint16_t *)header)[1];
    printf("\nserver checsum: %d\n", server_checksum);

    uint32_t length_text = ntohl(((uint32_t *)header)[1]);
    printf("\nlength: %d\n", length_text);

    while (output_bytes < (length_text - 8)){
        if ((received_bytes = recv(sockfd, buf, MAXDATASIZE-8, 0)) == -1) {
            perror("recv");
            exit(1);
        }
        for (int i=0; i<received_bytes; i++){
            output[output_bytes + i] = buf[i];
        }
        output_bytes += received_bytes;
    }
    
    int16_t *hp = (int16_t *)header;
    ++hp; *hp = 0;
    uint16_t checksumValid = checksum2(header, (char *)output, length_text - 8);
    printf("\nvalid checksum: %d\n", checksumValid);

    if (server_checksum != checksumValid){
        printf("\nchecksum not valid\n");
        return -1;
    }

    printf("\n numbytes: %d\n", output_bytes);
    buf[output_bytes] = '\0';
    printf("\nclient: received '%s'\n", output);

    close(sockfd);

    return 0;
}