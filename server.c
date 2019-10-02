#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>

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
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    char buff[256];

    base_sock = open_socket(base_sock);

    if (base_sock == -1)
        printf("ERROR opening socket");
    printf("%d", base_sock);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(BASEPORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serv_addr.sin_zero), 8);

    if (bind_socket(base_sock, serv_addr) < 0)
        printf("ERROR on binding socket");

    clilen = sizeof(struct sockaddr_in);

    /*
     * Loop de recepção de dados de clientes:
     * Como fazer? Utilizar sistema de uso de ports diferentes,
     * simular o "accept"/"connect" do TCP?
    */

    int connection = 1;

    while (connection)
    {
        /* receive from socket */
        n = recvfrom(base_sock, buf, 256, 0, (struct sockaddr *) &cli_addr, &clilen);
        if (n < 0)
            printf("ERROR on receive");
        printf("Received a datagram: %s\n", buf);

        /* send to socket */
        n = sendto(base_sock, "Got message\n", 17, 0, (struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
        if (n < 0)
            printf("ERROR on send");
    }

    close(base_sock);
    return 0;
}