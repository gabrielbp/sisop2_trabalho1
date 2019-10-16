#include "clientUtils.cpp"

#define SOCKET_BUFFER_LEN 1024
#define SOCKET_DEBUG_PORT 4000


void* ReceivedDatagram(T_SocketMessageData msgData){
        printf("\nRecebi um datagrama!!!");
        printf("\n%s", msgData.message.payload);
        if(msgData.message.type != MessageType::ACK)
            SendAcknowledgeMessage(msgData.socketId, msgData.senderAddress, msgData.senderAddressLength);
    //exit(0);
}

int main(int argc, char *argv[]) {
    if(argc < 4){
        printf("Usage: Client hostname username port\n");
        exit(0);
    }
    StartClient(argv[1], argv[2], atoi(argv[3]), ReceivedDatagram);
}