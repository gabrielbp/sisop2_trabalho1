#include "include/sockets/sockets.h"
#include "include/threads/threads.h"
#include <unistd.h>

#define SOCKET_DEBUG_PORT 4000

int socketId;

void* ReceivedDatagram(T_SocketMessageData msgData){
    
    printf("\nRecebi um datagrama!!!");
    printf("\n%s", msgData.message);
    SendAcknowledgeMessage(socketId, msgData.senderAddress, msgData.senderAddressLength);
}

int main(int argc, char *argv[]){
    
    socklen_t fromlen;
    struct sockaddr_in from;
    char buffer[SOCKET_BUFFER_LEN];
    int received_bytes;
    int sent_bytes;

    socketId = NewServerSocket(SOCKET_DEBUG_PORT);
    if(socketId == -1){
        printf("Erro criando socket");
    }

    T_WaitForMessageInSocketParameter waitForMsgParam;
    waitForMsgParam.socketId = socketId;
    waitForMsgParam.messageReceivedCallback = ReceivedDatagram;

    createNewThread(WaitForMessageInSocket, (void*)&waitForMsgParam);

    while(1){  
    }

}