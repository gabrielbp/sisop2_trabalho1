#include "sockets.h"

int connectSocketId;                                         //id do socket do servidor que espera por novas conexões
void* (*serverMessageReceivedCallback)(T_SocketMessageData);    //callback chamada sempre que o servidor recebe uma requisição
T_SocketConnection connectionsList[NUM_MAX_CONNECTIONS];    //Lista de todas as conexões ativas correntes com o servidor

/*
    Dado o identificador de um socket inicializado e uma porta, vincula o socket à porta
    Retorna -1 se houve erro e 0 se executou com sucesso
*/
int BindNewSocket(int socketId, int port){
    struct sockaddr_in newSocketAddress;
    int addressLength = sizeof(newSocketAddress);
    int bind_status;

    newSocketAddress.sin_family = AF_INET; //o socket é da família INET pois as conexões são através da internet
    newSocketAddress.sin_addr.s_addr= INADDR_ANY; //aqui dizemos que o socket pode ficar em qualquer endereço disponível
    newSocketAddress.sin_port = htons(port); //aqui definimos a porta onde ficará o socket (htons converte um inteiro para network byte order)

    bind_status = bind(socketId, (struct sockaddr *)&newSocketAddress, addressLength); //vincula o socket especificado pelo handle socketId ao endereço especificado por newSocketAddress
    if(bind_status < 0){
        printf("[BindNewSocket] Erro ao vincular o socket");
        return -1;
    } else {
        return 0;
    }
}

/*
    Dada uma porta, cria um novo socket de servidor vinculado a esta porta.
    Retorna o identificador deste socket se executou com sucesso, -1 se houve um erro
*/
int NewServerSocket(int port){
    int socketId;
    socklen_t length;
    struct sockaddr_in server;

    socketId = socket(AF_INET, SOCK_DGRAM, DEFAULT_OPTIONS);
    if(socketId < 0){
        printf("[NewServerSocket] Erro ao criar o socket");
    }

    if(BindNewSocket(socketId, port) < 0){
        return -1;
    } else {
        return socketId;
    }
}

/*
    Cria um novo socket de cliente
    Retorna o identificador do socket se executou com sucesso ou -1 se houve algum erro
*/
int NewClientSocket(){
    int socketId = socket(AF_INET, SOCK_DGRAM, DEFAULT_OPTIONS);
    if(socketId < 0){
        printf("[NewClientSocket] Erro ao criar o socket");
        return -1;
    } else {
        return socketId;
    }
}

/*
    Dados um nome de host, uma porta e um buffer para a estrutura de endereço de socket,
    preenche o buffer com as informações do servidor, se encontrar
    Retorna 0 se executou com sucesso e -1 se houve algum erro
*/
int GetServerAddress(char *hostname, int port, struct sockaddr_in *serverAddress){
    struct sockaddr_in server;
    struct hostent *host_info;

    /*
        struct hostent *gethostbyname(const char *name);
    Dado um nome de servidor (que pode ser um endereço IPv4 ou uma string tipo localhost ou www.google.com),
    retorna uma struct hostent com informações sobre o host

    struct hostent {
        char *h_name;       // official name of host //
        char **h_aliases;   // alias list //
        int h_addrtype;     // host address type //
        int h_length;       // length of address //
        char **h_addr_list; // list of addresses //
    };
    //for backwards compatibility: 
        #define h_addr h_addr_list[0]
    */
    host_info = gethostbyname(hostname);
    if(host_info == 0){
        printf("[GetServerAddress] Erro: Host desconhecido");
        return -1;
    }

    bcopy((char *)host_info->h_addr, (char *)&serverAddress->sin_addr, host_info->h_length); //apenas copia o endereço descoberto para a struct que define o servidor
    serverAddress->sin_family = AF_INET; //o socket é da família INET pois as conexões são através da internet
    serverAddress->sin_port = htons(port); //aqui definimos a porta onde ficará o socket (htons converte um inteiro para network byte order)
}

/*
    Dados o identificador de um socket e uma função de callback que receba uma T_SocketMessageData,
    espera por mensagens nesse socket e, quando chega uma, chama a função callback passando os dados
    da mensagem. A ideia é que esta função seja usada em uma thread própria
*/
void* WaitForMessageInSocket(void* socketIdAndCallback){
    T_WaitForMessageInSocketParameter input = *(T_WaitForMessageInSocketParameter*)socketIdAndCallback;
    int socketId = input.socketId;
    int received_bytes;
    char buffer[SOCKET_BUFFER_LEN];
    T_SocketMessageData currentMessageData;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    
    while(1){

        /*
            ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
        
        Coloca no buffer buf de tamanho len uma mensagem recebida no socket sockfd. Preenche src_addr com a struct que define o 
        socket remetente e addrlen com o tamanho dessa struct. As flags são usadas para definir o comportamento da função 
        (por exemplo, se é ou não bloqueante) 
        */
        received_bytes = recvfrom(socketId, buffer, SOCKET_BUFFER_LEN, DEFAULT_OPTIONS, (struct sockaddr *)&currentMessageData.senderAddress,&currentMessageData.senderAddressLength);
        if(received_bytes < 0){
            printf("[WaitForMessageInSocket] Erro ao receber mensagem");
        }

        bcopy(buffer, currentMessageData.message, received_bytes);
        currentMessageData.message_length = received_bytes;

        input.messageReceivedCallback(currentMessageData);
    }


}

/*
    Rotina para envio de uma mensagem, baseada no sistema de ACKs do protocolo TCP
    É dividida em 2 partes: o envio da mensagem e o recebimento do ACK que confirma que a mensagem foi recebida
    Primeiro, a rotina envia a mensagem, então fica em busy waiting esperando receber o ACK
    Se não receber até TIMEOUT_SECONDS segundos, reenvia a mensagem    
*/
void* SendMessageInSocket(void* socketIdAndMessageData){
    
    T_SendMessageInSocketParameter input = *(T_SendMessageInSocketParameter*)socketIdAndMessageData;
    int socketId = input.socketId;
    T_SocketMessageData messageData = input.messageData;
    std::chrono::duration<double> elapsed_seconds = (std::chrono::duration<double>)0.0;
    auto waitStartTime = std::chrono::system_clock::now();
    int received_bytes;
    char ackBuffer[ACKNOWLEDGE_MSG_LEN];
    int gotAcknowledgeMessage = 0;

    while(gotAcknowledgeMessage == 0){
        /*
        Envio da mensagem
        */

        /*
            ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
        Envia a mensagem de buf (de tamanho len) para o endereço definido por dest_addr (com tamanho addrlen) através do socket sockfd.
        As flags definem comportamentos da função
        */
        int sent_bytes = sendto(socketId, messageData.message, messageData.message_length, DEFAULT_OPTIONS, (struct sockaddr *)&messageData.senderAddress, messageData.senderAddressLength);
        if(sent_bytes < 0){
            printf("[SendMessageInSocket] Erro ao enviar mensagem");
            return nullptr;
        }

        /*
            Recebimento do ACK
        */
        waitStartTime = std::chrono::system_clock::now();
        elapsed_seconds = (std::chrono::duration<double>)0.0;
        while(elapsed_seconds < (std::chrono::duration<double>)TIMEOUT_SECONDS){
            received_bytes = recvfrom(socketId, ackBuffer, ACKNOWLEDGE_MSG_LEN, MSG_DONTWAIT, (struct sockaddr *)&messageData.senderAddress,&messageData.senderAddressLength);
            if(received_bytes != -1){
                gotAcknowledgeMessage = 1;
                break;
            }
            elapsed_seconds = std::chrono::system_clock::now() - waitStartTime;
        }
    }

    return nullptr;

}


/*
    Dados um identificador de um socket já inicializado, um endereço de destino e o tamanho desse endereço,
    envia uma mensagem de ACK para o endereço
    Retorna 0 se executou com sucesso e -1 se houve algum erro
*/
int SendAcknowledgeMessage(int socketId, struct sockaddr_in destinationAddress, socklen_t destinationAddressLength){   
    if(socketId == CONNECT_SOCKET) socketId = connectSocketId;
    int sent_bytes = sendto(socketId, ACKNOWLEDGE_MSG, ACKNOWLEDGE_MSG_LEN, 0, (struct sockaddr *)&destinationAddress, destinationAddressLength);
    
    if(sent_bytes < 0){
        //printf("[SendMessageInSocket] Erro ao enviar mensagem de ack");
        return -1;
    } else {
        return 0;
    }
}

/*
    Dado um identificador de socket, retorna a estrutura referente ao endereço deste socket
    ou -1 se houver algum erro
*/
struct sockaddr_in GetSocketAddress(int socketId){
    struct sockaddr_in socketAddress;
    socklen_t addressLength = sizeof(socketAddress);

    getsockname(socketId, (struct sockaddr *)&socketAddress, &addressLength);
    return socketAddress;
}


/*
    Dados o hostname, o nome de usuário, um identificador de socket inicializado e uma porta, executa o procedimento de conexão
    com o servidor.
    Retorna o número da porta que deverá ser usada pelas próximas requisições ao servidor se executou com sucesso
    Se houve erro, retorna -1
*/
int Connect(char* hostname, char* username, int socketId, int port){
    struct sockaddr_in serverAddress;
    socklen_t addressLength = sizeof(serverAddress);
    char buffer[12];

    if(GetServerAddress(hostname, port, &serverAddress) == -1){
        return -1;
    }

    T_SendMessageInSocketParameter sendMsgParam;
    sendMsgParam.socketId = socketId;
    bcopy(username, sendMsgParam.messageData.message, strlen(username));
    sendMsgParam.messageData.message_length = strlen(username);
    sendMsgParam.messageData.senderAddress = serverAddress;
    sendMsgParam.messageData.senderAddressLength = sizeof(serverAddress);

    SendMessageInSocket((void*)&sendMsgParam);

    int received_bytes = recvfrom(socketId, buffer, 12, DEFAULT_OPTIONS, (struct sockaddr *)&serverAddress, &addressLength);
    if(received_bytes < 0){
        printf("[Connect] Erro ao receber mensagem de accept");
        return -1;
    }

    int porta = atoi(buffer);

    return ntohs(porta); //o valor da porta vai estar em network byte order e precisa ser passado para host byte order

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
    Dado um endereço de socket e um nome de usuário, percorre a lista de conexões checando se já existe uma conexão com este socket e este usuário
    Se existir, retorna o índice desta conexão no array de conexões
    Se não existir, retorna -1
*/
int FindConnection(struct sockaddr_in connectionAddress, char* username){
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
    Esta função é chamada quando acontecem novas requisições no socket connect, considerando que essas requisições terão como dados apenas o nome de usuário
    Primeiro checa se já existe um socket criado especificamente para esta conexão
    Se já existe, manda a porta em que está vinculada este socket de volta ao remetente
    Se não existe, cria um novo socket, envia a a porta em que está vinculada este socket e cria uma nova thread 
    para ficar monitorando as requisições no novo socket
*/
void* Accept(T_SocketMessageData connectionData){
    int connectionIndex = FindConnection(connectionData.senderAddress, connectionData.message);
    SendAcknowledgeMessage(connectSocketId, connectionData.senderAddress, connectionData.senderAddressLength);

    if(connectionIndex == -1){
        //Esta conexão ainda não foi criada, então deve ser criada e deve-se enviar um datagrama com os dados da nova conexão
        int newConnectionIndex = FindConnectionSlot();
        if(newConnectionIndex == -1){
            return nullptr;
        }
        struct sockaddr_in newSocketAddress;    //endereço vinculado ao socket que será enviado de volta ao cliente, para que este possa se comunicar com o novo socket

        connectionsList[newConnectionIndex].isValid = 1;
        char* username = (char*)malloc(sizeof(char)*256);
        connectionsList[newConnectionIndex].username = username;
        strcpy(connectionsList[newConnectionIndex].username, connectionData.message);
        connectionsList[newConnectionIndex].connectionAddress = connectionData.senderAddress;
        connectionsList[newConnectionIndex].connectionAddressLength = connectionData.senderAddressLength;

        int newSocketId = NewServerSocket(ANY_PORT);
        if(newSocketId == -1){
            return nullptr;
        }
        printf("\n============newSocketId: %d\n\n", newSocketId);

        connectionsList[newConnectionIndex].socketId = newSocketId;
        newSocketAddress = GetSocketAddress(newSocketId);   //recupera o endereço do novo socket para enviar ao cliente

        char str_port[12];
        sprintf(str_port, "%d", newSocketAddress.sin_port);

        int sent_bytes = sendto(newSocketId, str_port, 12, DEFAULT_OPTIONS, (struct sockaddr *)&connectionData.senderAddress, connectionData.senderAddressLength);
        if(sent_bytes < 0){
            connectionsList[newConnectionIndex].isValid = false;
            close(connectionsList[newConnectionIndex].socketId);
            return nullptr;
        } else {
            T_WaitForMessageInSocketParameter waitForMsgParam;
            waitForMsgParam.socketId = newSocketId;
            waitForMsgParam.messageReceivedCallback = serverMessageReceivedCallback;
            connectionsList[newConnectionIndex].threadId = createNewThread(WaitForMessageInSocket, (void*)&waitForMsgParam);
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
            return nullptr;
        }
    }

    return nullptr;
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
        printf("[StartServer] Erro criando socket");
    }

    T_WaitForMessageInSocketParameter waitForMsgParam;
    waitForMsgParam.socketId = connectSocketId;
    waitForMsgParam.messageReceivedCallback = Accept;

    createNewThread(WaitForMessageInSocket, (void*)&waitForMsgParam);
}