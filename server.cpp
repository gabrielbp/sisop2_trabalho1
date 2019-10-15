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
    if(strcmp(msgData.message, "QUIT\n") == 0){
        int connectionIndex = FindConnection(msgData.senderAddress, DEBUG_USERNAME);
        if(connectionIndex == -1){
        } else {
            DeleteConnection(connectionIndex);
        }
    }
}

int main(int argc, char *argv[]){

    /*Inicializa os dados do servidor e a thread de conexão
    que fica lendo o socket de conexão esperando pelos clientes*/
    StartServer(SOCKET_DEBUG_PORT, ReceivedDatagram);

    while(1){
        
    }
}


