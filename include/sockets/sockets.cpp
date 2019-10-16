#include "sockets.h"
#include <errno.h>

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
    Rotina para envio de uma mensagem, baseada no sistema de ACKs do protocolo TCP
    É dividida em 2 partes: o envio da mensagem e o recebimento do ACK que confirma que a mensagem foi recebida
    Primeiro, a rotina envia a mensagem, então fica em busy waiting esperando receber o ACK
    Se não receber até TIMEOUT_SECONDS segundos, reenvia a mensagem    

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

        int sent_bytes = sendto(socketId, (void*)&messageData.message, sizeof(T_Message), DEFAULT_OPTIONS, (struct sockaddr *)&messageData.senderAddress, messageData.senderAddressLength);
        if(sent_bytes < 0){
            printf("[SendMessageInSocket] Erro ao enviar mensagem");
            return nullptr;
        }

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
*/


/*
    Dados um identificador de um socket já inicializado, um endereço de destino e o tamanho desse endereço,
    envia uma mensagem de ACK para o endereço
    Retorna 0 se executou com sucesso e -1 se houve algum erro
*/
int SendAcknowledgeMessage(int socketId, struct sockaddr_in destinationAddress, socklen_t destinationAddressLength){   
    
    T_Message ackMessage;
    ackMessage.type = MessageType::ACK;
    
    int sent_bytes = sendto(socketId, (void*)&ackMessage, sizeof(T_Message), DEFAULT_OPTIONS, (struct sockaddr *)&destinationAddress, destinationAddressLength);
    
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