#include "include/sockets/sockets.h"

#define SOCKET_BUFFER_LEN 1024
#define SOCKET_DEBUG_PORT 4000
#define HOST_NAME "localhost"
#define DEBUG_USERNAME "usernameABC"

int main(int argc, char *argv[]){
    int socketId;
    struct sockaddr_in serverAddress;
    struct sockaddr_in senderAddress;
    char buffer[SOCKET_BUFFER_LEN];

    puts("\nConectando...");

    socketId = NewClientSocket();
    if(socketId == -1){
        printf("Erro ao criar o socket");
    }

    int porta_servidor = Connect(HOST_NAME, DEBUG_USERNAME, socketId, SOCKET_DEBUG_PORT);
    printf("\nConectado!");

    if(GetServerAddress(HOST_NAME, porta_servidor, &serverAddress) == -1){
        printf("Erro ao achar o host");
    }

    printf("\nEntre a mensagem a ser enviada: \n");
    bzero(buffer, SOCKET_BUFFER_LEN); //apenas zera o buffer para evitar bugs
    fgets(buffer,SOCKET_BUFFER_LEN-1, stdin);   //lê uma string do console


    T_SendMessageInSocketParameter sendMsgParam;
    sendMsgParam.socketId = socketId;
    bcopy(buffer, sendMsgParam.messageData.message, SOCKET_BUFFER_LEN);
    sendMsgParam.messageData.message_length = SOCKET_BUFFER_LEN;
    sendMsgParam.messageData.senderAddress = serverAddress;
    sendMsgParam.messageData.senderAddressLength = sizeof(serverAddress);

    SendMessageInSocket((void*)&sendMsgParam);
    
}