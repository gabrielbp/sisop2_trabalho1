#include "utilsUtils.cpp"

MessageEventsSource eventsSource;

int connectSocketId;                                         //id do socket do servidor que espera por novas conexões
void* (*serverMessageReceivedCallback)(T_SocketMessageData);    //callback chamada sempre que o servidor recebe uma requisição
T_SocketConnection connectionsList[NUM_MAX_CONNECTIONS];    //Lista de todas as conexões ativas correntes com o servidor

/*
Dados o identificador de um socket e uma função de callback que receba uma T_SocketMessageData,
espera por mensagens nesse socket e, quando chega uma, chama a função callback passando os dados
da mensagem. A ideia é que esta função seja usada em uma thread própria
*/
void* WaitForMessageInSocket(void* _socketId){
    int socketId = *(int*)_socketId;
    int received_bytes;
    T_Message message;
    T_SocketMessageData currentMessageData;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    while(1){

        /*
            ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
        
        Coloca no buffer buf de tamanho len uma mensagem recebida no socket sockfd. Preenche src_addr com a struct que define o 
        socket remetente e addrlen com o tamanho dessa struct. As flags são usadas para definir o comportamento da função 
        (por exemplo, se é ou não bloqueante) 
        */
        received_bytes = recvfrom(socketId, (void*)&message, sizeof(T_Message), DEFAULT_OPTIONS, (struct sockaddr *)&currentMessageData.senderAddress,&currentMessageData.senderAddressLength);
        if(received_bytes < 0){
            printf("\n[WaitForMessageInSocket] Erro ao receber mensagem");
            if(errno == EBADF){
                printf("\nBAD FILE DESCRIPTOR");
            }
        } else {
            currentMessageData.message = message;
            currentMessageData.socketId = socketId;
            eventsSource.PublishReceivedNewMessage(currentMessageData, socketId);
        }
    }


}

    /*
    Percorre a lista de conexões, inicializando todas elas como inválidas
    */
    void InitializeConnectionsList(){
        int i = 0;
        for(i = 0; i < NUM_MAX_CONNECTIONS; i++){
            connectionsList[i].isValid = 0;
        }
    }

    /*
        Percorre a lista de conexões buscando uma conexão inválida que poderá ser sobrescrita, ou seja, um slot de conexão vazio
        Retorna o índice desse slot, se encontrar
        Se não encontrar, retorna -1
    */
    int FindConnectionSlot(){
        int i = 0;
        for(i = 0; i < NUM_MAX_CONNECTIONS; i++){
            if(connectionsList[i].isValid == 0){
                return i;
            }
        }
        return -1;
    }

    /*
        Dado o índice de uma conexão no array de conexões, fecha o socket e marca ela como inválida
        Não tem retorno
    */
    void DeleteConnection(int connectionIndex){
        close(connectionsList[connectionIndex].socketId);
        connectionsList[connectionIndex].isValid = 0;
        killThread(connectionsList[connectionIndex].threadId);
    }

    /*
        Dado um índice de uma conexão na lista de conexões, retorna ela
    */
    T_SocketConnection GetConnection(int connectionIndex){
        return connectionsList[connectionIndex];
    }

    /*
        Dado um endereço de socket e um nome de usuário, percorre a lista de conexões checando se já existe uma conexão com este socket e este usuário
        Se existir, retorna o índice desta conexão no array de conexões
        Se não existir, retorna -1
    */
    int FindConnectionIndex(struct sockaddr_in connectionAddress, char* username){
        int i = 0;
        T_SocketConnection currentConnection;

        for(i = 0; i<NUM_MAX_CONNECTIONS; i++){
            currentConnection = connectionsList[i];

            if( currentConnection.isValid == 1 &&                                                           //Checa se a conexão é válida
                strcmp(username, currentConnection.username) == 0 &&                                        //checa se o username é o mesmo
                connectionAddress.sin_addr.s_addr == currentConnection.connectionAddress.sin_addr.s_addr)   //checa se o endereço é o mesmo
                {
                    //Se for o caso, encontramos a conexão desejada e podemos apenas retornar i
                    return i;
                }
        }
        return -1;
    }

    /*
        Esta função deve ser executada em uma thread própria
        Fica no socket Connect esperando por novas requisições de conexão
        Quando detecta uma, lança o evento ReceivedNewConnection com as informações da mensagem
    */
    void* WaitForConnectionRequest(void* a){
        int received_bytes;
        char buffer[SOCKET_BUFFER_LEN];
        T_SocketMessageData currentMessageData;

        while(1){

            /*
                ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
            
            Coloca no buffer buf de tamanho len uma mensagem recebida no socket sockfd. Preenche src_addr com a struct que define o 
            socket remetente e addrlen com o tamanho dessa struct. As flags são usadas para definir o comportamento da função 
            (por exemplo, se é ou não bloqueante) 
            */
            received_bytes = recvfrom(connectSocketId, buffer, SOCKET_BUFFER_LEN, DEFAULT_OPTIONS, (struct sockaddr *)&currentMessageData.senderAddress,&currentMessageData.senderAddressLength);
            if(received_bytes < 0){
                printf("\n[WaitForMessageInSocket] Erro ao receber mensagem");
                if(errno == EBADF){
                    printf("\nBAD FILE DESCRIPTOR");
                }
            } else {
                bcopy(buffer, (void*)&currentMessageData.message, sizeof(T_Message));
                eventsSource.PublishReceivedNewConnection(currentMessageData);
            }
        }


    }

    /*
        Esta função deve ser vinculada ao evento ReceivedNewConnection, ou seja, é chamada quando há uma nova requisição de conexão
        Primeiro checa se já existe um socket criado especificamente para esta conexão que foi requisitada
        Se já existe, manda a porta em que está vinculada este socket de volta ao remetente
        Se não existe, cria um novo socket, envia a a porta em que está vinculada este socket e cria uma nova thread 
        para ficar monitorando as requisições no novo socket
        Esta função deve estar vinculada ao evento de nova mensagem no socket Connect
    */
    void Accept(T_SocketMessageData connectionData){        

        int connectionIndex = FindConnectionIndex(connectionData.senderAddress, connectionData.message.payload);
        SendAcknowledgeMessage(connectSocketId, connectionData.senderAddress, connectionData.senderAddressLength);
        if(connectionIndex == -1){
            //Esta conexão ainda não foi criada, então deve ser criada e deve-se enviar um datagrama com os dados da nova conexão
            int newConnectionIndex = FindConnectionSlot();
            if(newConnectionIndex == -1){
                return;
            }
            struct sockaddr_in newSocketAddress;    //endereço vinculado ao socket que será enviado de volta ao cliente, para que este possa se comunicar com o novo socket

            connectionsList[newConnectionIndex].isValid = 1;
            char* username = (char*)malloc(sizeof(char)*256);
            connectionsList[newConnectionIndex].username = username;
            strcpy(connectionsList[newConnectionIndex].username, connectionData.message.payload);
            connectionsList[newConnectionIndex].connectionAddress = connectionData.senderAddress;
            connectionsList[newConnectionIndex].connectionAddressLength = connectionData.senderAddressLength;

            int newSocketId = NewServerSocket(ANY_PORT);
            if(newSocketId == -1){
                return;
            }

            connectionsList[newConnectionIndex].socketId = newSocketId;
            newSocketAddress = GetSocketAddress(newSocketId);   //recupera o endereço do novo socket para enviar ao cliente

            T_Message portMessage;
            portMessage.type = MessageType::DEFINE_PORT;
            sprintf(portMessage.payload, "%d", newSocketAddress.sin_port);

            int sent_bytes = sendto(newSocketId, (void*)&portMessage, sizeof(T_Message), DEFAULT_OPTIONS, (struct sockaddr *)&connectionData.senderAddress, connectionData.senderAddressLength);
            if(sent_bytes < 0){
                connectionsList[newConnectionIndex].isValid = false;
                close(connectionsList[newConnectionIndex].socketId);
                return;
            } else {
                connectionsList[newConnectionIndex].threadId = createNewThread(WaitForMessageInSocket, (void*)&newSocketId);
                eventsSource.SubscribeToReceivedNewMessage(serverMessageReceivedCallback, newSocketId);
            }

        } else {
            //Esta conexão já foi criada, então o datagrama com os dados da nova conexão se perdeu e deve ser reenviado
            T_SocketConnection connection = connectionsList[connectionIndex];

            struct sockaddr_in socketAddress = GetSocketAddress(connection.socketId);
            socklen_t socketAddressLength = sizeof(socketAddress);

            char str_port[12];
            sprintf(str_port, "%d", socketAddress.sin_port);

            //Envia para o cliente a porta em que se encontra o novo socket
            int sent_bytes = sendto(connection.socketId, str_port, 12, DEFAULT_OPTIONS, (struct sockaddr *)&connectionData.senderAddress, connectionData.senderAddressLength);
            if(sent_bytes < 0){
                return;
            }
        }

        return;
    }

/*
    Dada a porta onde o socket de conexão rodará (que pode ser 0 se não faz diferença) e uma callback pra quando chegar
    uma requisição, inicializa o servidor
*/
void StartServer(int port, void* (*_messageReceivedCallback)(T_SocketMessageData)){
    serverMessageReceivedCallback = _messageReceivedCallback;
    
    InitializeConnectionsList();
    
    connectSocketId = NewServerSocket(port);
    if(connectSocketId == -1){
        printf("\n[StartServer] Erro criando socket");
        exit(-1);
    }

    if(createNewThread(WaitForConnectionRequest, nullptr) == -1){
        printf("\n[StartServer] Erro criando thread do socket connect");
        exit(-1);
    }

    std::function<void(T_SocketMessageData)> foo = Accept;

    eventsSource.SubscribeToReceivedNewConnection(foo);
}

int gotAcknowledgeMessage = 0;

void GotAckMessage(T_SocketMessageData msgData){
    if(msgData.message.type == MessageType::ACK){
        gotAcknowledgeMessage = 1;
    }
}

void* SendMessageInSocket(void* messageData){

    T_SocketMessageData msgData = *(T_SocketMessageData*)messageData;

    std::chrono::duration<double> elapsed_seconds = (std::chrono::duration<double>)0.0;
    auto waitStartTime = std::chrono::system_clock::now();
    gotAcknowledgeMessage = 0;

    int subscriptionId = eventsSource.SubscribeToReceivedNewMessage(GotAckMessage, msgData.socketId);

    while(gotAcknowledgeMessage == 0){
        /*
            ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
        Envia a mensagem de buf (de tamanho len) para o endereço definido por dest_addr (com tamanho addrlen) através do socket sockfd.
        As flags definem comportamentos da função
        */
        int sent_bytes = sendto(msgData.socketId, (void*)&msgData.message, sizeof(T_Message), DEFAULT_OPTIONS, (struct sockaddr *)&msgData.senderAddress, msgData.senderAddressLength);
        if(sent_bytes < 0){
            printf("[SendMessageInSocket] Erro ao enviar mensagem");
            return nullptr;
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