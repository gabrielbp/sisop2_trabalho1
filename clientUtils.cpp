#include "utilsUtils.cpp"

#define SOCKET_DEBUG_PORT 4000

MessageEventsSource eventsSource;
int clientSocketId;

int gotAcknowledgeMessage = 0;

void GotAckMessage(T_SocketMessageData msgData){
    if(msgData.message.type == MessageType::ACK){
        gotAcknowledgeMessage = 1;
    }
}

void SendMessageInSocket(T_SocketMessageData messageData){

    std::chrono::duration<double> elapsed_seconds = (std::chrono::duration<double>)0.0;
    auto waitStartTime = std::chrono::system_clock::now();
    gotAcknowledgeMessage = 0;

    int subscriptionId = eventsSource.SubscribeToReceivedNewMessage(GotAckMessage, clientSocketId);

    while(gotAcknowledgeMessage == 0){
        /*
            ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
        Envia a mensagem de buf (de tamanho len) para o endereço definido por dest_addr (com tamanho addrlen) através do socket sockfd.
        As flags definem comportamentos da função
        */
        int sent_bytes = sendto(clientSocketId, (void*)&messageData.message, sizeof(T_Message), DEFAULT_OPTIONS, (struct sockaddr *)&messageData.senderAddress, messageData.senderAddressLength);
        if(sent_bytes < 0){
            printf("[SendMessageInSocket] Erro ao enviar mensagem");
            return;
        }

        waitStartTime = std::chrono::system_clock::now();
        elapsed_seconds = (std::chrono::duration<double>)0.0;
        while(elapsed_seconds < (std::chrono::duration<double>)TIMEOUT_SECONDS){
            if(gotAcknowledgeMessage == 1)
                break;
            elapsed_seconds = std::chrono::system_clock::now() - waitStartTime;
        }
    }
    eventsSource.UnsubscribeToReceivedNewMessage(subscriptionId);
}

    /*
    Dados o identificador de um socket e uma função de callback que receba uma T_SocketMessageData,
    espera por mensagens nesse socket e, quando chega uma, chama a função callback passando os dados
    da mensagem. A ideia é que esta função seja usada em uma thread própria
*/
void* WaitForMessageInSocket(void* a){
    int received_bytes;
    T_Message message;;
    T_SocketMessageData currentMessageData;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    while(1){

        /*
            ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
        
        Coloca no buffer buf de tamanho len uma mensagem recebida no socket sockfd. Preenche src_addr com a struct que define o 
        socket remetente e addrlen com o tamanho dessa struct. As flags são usadas para definir o comportamento da função 
        (por exemplo, se é ou não bloqueante) 
        */
        received_bytes = recvfrom(clientSocketId, (void*)&message, sizeof(T_Message), DEFAULT_OPTIONS, (struct sockaddr *)&currentMessageData.senderAddress,&currentMessageData.senderAddressLength);
        if(received_bytes < 0){
            printf("\n[WaitForMessageInSocket] Erro ao receber mensagem");
            if(errno == EBADF){
                printf("\nBAD FILE DESCRIPTOR");
            }
        } else {
            currentMessageData.message = message;
            currentMessageData.socketId = clientSocketId;
            printf("\nmensagem recebida: %s\n", currentMessageData.message.payload);
            eventsSource.PublishReceivedNewMessage(currentMessageData, clientSocketId);
            
        }
    }


}

int serverPort = -1;

void GotPortNumber(T_SocketMessageData msgData){
    if(msgData.message.type == MessageType::DEFINE_PORT){
        serverPort = atoi(msgData.message.payload);
    }
}

/*
    Dados o hostname, o nome de usuário, um identificador de socket inicializado, uma porta e uma função callback, executa o procedimento de conexão
    com o servidor e inicia uma thread que ficará lendo no socket.
    Retorna o número da porta que deverá ser usada pelas próximas requisições ao servidor se executou com sucesso
    Se houve erro, retorna -1
*/
int Connect(char* hostname, char* username, int port, void* (*messageReceivedCallback)(T_SocketMessageData)){
    struct sockaddr_in serverAddress;
    socklen_t addressLength = sizeof(serverAddress);
    char buffer[12];

    if(GetServerAddress(hostname, port, &serverAddress) == -1){
        return -1;
    }

    T_SocketMessageData connectMsgData;
    bcopy(username, (void*)&connectMsgData.message, strlen(username));
    connectMsgData.senderAddress = serverAddress;
    connectMsgData.senderAddressLength = sizeof(serverAddress);

    if(createNewThread(WaitForMessageInSocket, (void*)&clientSocketId) == -1){
        printf("[Connect] Erro ao criar a thread");
    }

    int subId = eventsSource.SubscribeToReceivedNewMessage(GotPortNumber, clientSocketId);

    SendMessageInSocket(connectMsgData);

    while(serverPort == -1){}   //fica esperando o servidor mandar o datagrama com o número da porta com a qual o cliente deve se comunicar

    eventsSource.UnsubscribeToReceivedNewMessage(subId);

    eventsSource.SubscribeToReceivedNewMessage(messageReceivedCallback, clientSocketId);

    return ntohs(serverPort); //o valor da porta vai estar em network byte order e precisa ser passado para host byte order

}

void StartClient(char* hostname, char* username, int port, void *(*messageReceivedCallback)(T_SocketMessageData)){
    struct sockaddr_in serverAddress;
    struct sockaddr_in senderAddress;
    char buffer[SOCKET_BUFFER_LEN];
    bool quit = false;

    
    puts("\nConectando...");

    clientSocketId = NewClientSocket();
    if(clientSocketId == -1){
        printf("Erro ao criar o socket");
    }

    int porta_servidor = Connect(hostname, username, port, messageReceivedCallback);
    puts("\nConectado!");

    if(GetServerAddress(hostname, porta_servidor, &serverAddress) == -1){
        printf("\nErro ao achar o host\n");
        exit(0);
    }

    while(1)
    {
        printf("\nEntre a mensagem a ser enviada: \n");
        bzero(buffer, SOCKET_BUFFER_LEN); //apenas zera o buffer para evitar bugs
        fgets(buffer,SOCKET_BUFFER_LEN-1, stdin);   //lê uma string do console

        T_SocketMessageData msgData;

        bcopy(buffer, (void*)&msgData.message.payload, SOCKET_BUFFER_LEN);
        msgData.senderAddress = serverAddress;
        msgData.senderAddressLength = sizeof(serverAddress);

        SendMessageInSocket(msgData);
    }
}