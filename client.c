#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define PORT 5000
#define MAXLINE 1024
int main(int argc, char **argv)
{
    int sockfd, nready, maxfd;
    ssize_t readlen;
    fd_set rset;
    char buffer[MAXLINE];
    struct sockaddr_in servaddr;

    if (argc < 2) {
        printf("\n Usage : ./client <server IP> \n");
        exit(1);
    }

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("socket creation failed");
        exit(0);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    if(inet_aton(argv[1], &(servaddr.sin_addr)) == 0) {
        printf("\n Error : Invalid IP address \n");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        printf("\n Error : Connect Failed \n");
    }

    FD_ZERO(&rset);

    maxfd = sockfd + 1;
    for (;;) {
        FD_SET(STDIN_FILENO, &rset);
        FD_SET(sockfd, &rset);
        // select the ready descriptor
        nready = select(maxfd, &rset, NULL, NULL, NULL);

        memset(buffer, 0, sizeof(buffer));
 
        // if tcp socket is readable then handle
        // it by accepting the connection
        if (FD_ISSET(sockfd, &rset)) {
            //printf("\n Info: Server readable! \n");
            if(read(sockfd, buffer, sizeof(buffer)) == 0) {
                close(sockfd);
                FD_CLR(sockfd, &rset);
                printf("\n Exit: Connection lost! \n");
                exit(0);
            }
            puts(buffer);
        }
        // if udp socket is readable receive the message.
        if (FD_ISSET(STDIN_FILENO, &rset)) {
            //printf("\n Info: stdin readable! \n");
            readlen = read(STDIN_FILENO, buffer, sizeof(buffer));
            write(sockfd, buffer, readlen);
        }
    }

    close(sockfd);

    return 0;
}