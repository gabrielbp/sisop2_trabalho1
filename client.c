#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BASEPORT 4000

int open_socket(int sock)
{
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    printf("%d \n", sock);
    return sock;
}

int bind_socket(int sock, struct sockaddr_in s_add)
{
    bind_result = bind(sock, (struct sockaddr *) &s_add, sizeof(struct sockaddr));
    return bind_result;
}

int main(int argc, char *argv[])
{
    int base_sock, n;
    unsigned int length;
    struct sockaddr_in serv_addr, from;
    struct hostent *server;

    char buffer[256];
    if (argc < 2)
    {
        fprintf(stderr, "usage %s hostname\n", argv[0]);
        exit(0);
    }

    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    base_sock = open_socket(base_sock);
    if (base_sock == -1)
        printf("ERROR opening socket");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(BASEPORT);
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);

    
}