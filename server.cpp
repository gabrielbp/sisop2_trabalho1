#include "serverUtils.cpp"

#define SOCKET_DEBUG_PORT 4000
#define DEBUG_USERNAME "amaury"


void* ReceivedDatagram(T_SocketMessageData msgData){
        printf("\nRecebi um datagrama!!!");
        printf("\n%s", msgData.message.payload);
        if(msgData.message.type != MessageType::ACK)
            SendAcknowledgeMessage(msgData.socketId, msgData.senderAddress, msgData.senderAddressLength);
        if(strcmp(msgData.message.payload, "sendBack\n") == 0){
            puts("VAI MANDAR DE VOLTA");
            createNewThread(SendMessageInSocket, (void*)&msgData);
        }
}

int main(int argc, char *argv[]) {
    if(argc < 2){
        printf("\nUsage: Server port\n");
        exit(0);
    }
   StartServer(atoi(argv[1]), ReceivedDatagram);
   
   while(1){}
}


