#include "include/sockets/sockets.h"

#define SOCKET_DEBUG_PORT 4000
#define DEBUG_USERNAME "usernameABC"


/*
    Esta função é uma callback que é chamada sempre que chega um datagrama em algum socket
*/
void* ReceivedDatagram(T_SocketMessageData msgData){
    printf("\nRecebi um datagrama!!!");
    printf("\n%s", msgData.message);
    SendAcknowledgeMessage(CONNECT_SOCKET, msgData.senderAddress, msgData.senderAddressLength);
    char *cmd = strtok(msgData.message, " \n");
    if(strcmp(cmd, "QUIT") == 0){
        puts("Got a QUIT");
        int connectionIndex = FindConnection(msgData.senderAddress, DEBUG_USERNAME);
        if(connectionIndex != -1){
            DeleteConnection(connectionIndex);
        }
    }
    if(strcmp(cmd, "upload") == 0){
        puts("Got an upload request");
    }
    if(strcmp(cmd, "download") == 0){
        puts("Got a download request");
    }
    if(strcmp(cmd, "delete") == 0){
        puts("Got a delete request");
    }
    if(strcmp(cmd, "list_server") == 0){
        puts("Got a list server request");
    }
    if(strcmp(cmd, "list_client") == 0){
        puts("Got a list client request");
    }
    if(strcmp(cmd, "get_sync_dir") == 0){
        puts("Got a get sync dir request");
    }
}

int main(int argc, char *argv[]){

    /*Inicializa os dados do servidor e a thread de conexão
    que fica lendo o socket de conexão esperando pelos clientes*/
    StartServer(SOCKET_DEBUG_PORT, ReceivedDatagram);

    while(1){
        
    }
}


