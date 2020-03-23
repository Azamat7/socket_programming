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

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZEIN 100 // max number of bytes we can get at once 
#define MAXDATASIZE 10 * 1000 * 1000

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    return &(((struct sockaddr_in*)sa)->sin_addr);
}

// void itos(int n, char * &s) { // works for positive numbers
// 	char **p = &s;

// 	while (1) {
// 		*p = n % 10;
// 		n /= 10;
		
// 		p++;
		
// 		if (n == 0) {
// 			break;
// 		}
// 	}
	
// 	*p = '\0';
// 	p--;
	
// 	char ** q = &s;
	
// 	while (q <= p) {
// 		swap(*q, *p);
// 		q++;
// 		p--;
// 	}
// }


// unsigned short checksum1(const char *buf, unsigned size)
// {
// 	unsigned sum = 0;
// 	int i;

// 	/* Accumulate checksum */
// 	for (i = 0; i < size - 1; i += 2)
// 	{
// 		unsigned short word16 = *(unsigned short *) &buf[i];
// 		sum += word16;
// 	}

// 	/* Handle odd-sized case */
// 	if (size & 1)
// 	{
// 		unsigned short word16 = (unsigned char) buf[i];
// 		sum += word16;
// 	}

// 	/* Fold to get the ones-complement result */
// 	while (sum >> 16) sum = (sum & 0xFFFF)+(sum >> 16);

// 	/* Invert to get the negative in ones-complement arithmetic */
// 	return ~sum;
// }


// -h 143.248.111.222 -p 1234 -o 0 -s 5


int main(int argc, char *argv[])
{
    int sockfd, numbytes;  
    char buf[1200];
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
                shift = atoi(argv[++i]);
                break;
            case 'h':
                host = argv[++i];
                break;
            default:
                printf("Please provide correct arguments: -h, -p, -o, -s\n");
                return 1;
        }
    }

    // int pSize = sysconf(_SC_PAGESIZE);
    // char *buffer = calloc(pSize, sizeof(char));
    // char *buffer = calloc(MAXDATASIZE, sizeof(char));
    // char buffer[MAXDATASIZE];
    // assert(buffer);

    char *message = (char*) malloc(MAXDATASIZE * sizeof(char));

    // checksum1(const char *buf, unsigned size)

    // 1008
    memcpy(message, "\1\5\1\2\0\3\15\0", 8);
    char *buffer = (char*) malloc(1000 * sizeof(char));
    // size = strlen(send_message + 4) + 1;

    int n;
    int length = 8;
    do {
        n = read(0, buffer, 1000);

        if ((length+n) >= MAXDATASIZE){
            printf("overflowed");
            return -1;
        }

        length += n;
        strncat(message, buffer, n);
        // process it
    } while(n > 0);
    // return 0;

    printf("%s\n\n", message);
    printf("%lu\n\n", strlen(message));

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

    printf("buffer size: %lu\n", strlen(buffer));
    
    uint32_t *pp = malloc(1008);
	uint32_t *qq = pp;
	
	*qq = htonl((0 << 0) + (5 << 16)); ++qq;
    // *qq = htonl(327680); ++qq;
	*qq = htonl(1008); ++qq;

    // for (int i=0; i<100; i++){
    //     printf("%d", pp[i]);
    // }
    printf("\n");

	for (int i = 0; i < 250; i++) {
		*qq = htonl((65 << 0) + (65 << 8) + (65 << 16) + (65 << 24)); ++qq;
	}
    
    for (int i=0; i<252; i++){
        printf("%d", pp[i]);
    }
    send(sockfd , pp , 1008 , 0 ); 
    
    // send(sockfd , message , strlen(message) , 0 ); 

    recv(sockfd, buf, 8, 0);
    if ((numbytes = recv(sockfd, buf, 1100, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    // return -1;
    // for (int i=0; i<100; i++){
    //     printf("%c\n", buf[i]);
    // }

    printf("\n numbytes: %d\n", numbytes);
    buf[numbytes] = '\0';

    printf("\nclient: received '%s'\n",buf);

    close(sockfd);

    return 0;
}