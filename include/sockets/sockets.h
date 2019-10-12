#ifndef SOCKETS
#define SOCKETS

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <strings.h>
#include <chrono>

#define NUM_MAX_CONNECTIONS 256
#define DEFAULT_OPTIONS 0
#define SOCKET_BUFFER_LEN 1024
#define TIMEOUT_SECONDS 5.0
#define ACKNOWLEDGE_MSG "ACK"
#define ACKNOWLEDGE_MSG_LEN 3

/*
    Uma estrutura que contém os dados referentes a uma mensagem recebida/enviada no socket
*/
typedef struct socketMessageData{
    char message[SOCKET_BUFFER_LEN];    //a mensagem em si, em um buffer de tamanho SUCKET_BUFFER_LEN
    int message_length;                 //o tamanho em bytes da mensagem
    struct sockaddr_in senderAddress;   //o endereço do remetente
    socklen_t senderAddressLength;      //o tamanho da estrutura do endereço do remetente
} T_SocketMessageData;


/*
    Estrutura de entrada para a função WaitForMessageInSocket()
*/
typedef struct waitForMessageInSocketParameter{
    int socketId;                                               //Identificador do socket no qual a função ficará esperando por mensagens
    void* (*messageReceivedCallback)(T_SocketMessageData);  //callback a ser chamada quando uma mensagem for recebida
} T_WaitForMessageInSocketParameter;


/*
    Estrutura de entrada para a função SendMessageInSocket()
*/
typedef struct sendMessageInSocketParameter{
    int socketId;
    socketMessageData messageData;
} T_SendMessageInSocketParameter;

/*
    Dada uma porta, cria um novo socket de servidor vinculado a esta porta.
    Retorna o identificador deste socket se executou com sucesso, -1 se houve um erro
*/
int NewServerSocket(int port);

/*
    Cria um novo socket de cliente
    Retorna o identificador do socket se executou com sucesso ou -1 se houve algum erro
*/
int NewClientSocket();

/*
    Dados um nome de host, uma porta e um buffer para a estrutura de endereço de socket,
    preenche o buffer com as informações do servidor, se encontrar
    Retorna 0 se executou com sucesso e -1 se houve algum erro
*/
int GetServerAddress(char *hostname, int port, struct sockaddr_in *serverAddress);


/*
    Dados o identificador de um socket e uma função de callback que receba uma T_MessageReceivedReport,
    espera por mensagens nesse socket e, quando chega uma, chama a função callback passando os dados
    da mensagem. A ideia é que esta função seja usada em uma thread secundária
*/
void* WaitForMessageInSocket(void* socketIdAndCallback);

/*
    Dados um identificador de um socket já inicializado, um endereço de destino e o tamanho desse endereço,
    envia uma mensagem de ACK para o endereço
    Retorna 0 se executou com sucesso e -1 se houve algum erro
*/
int SendAcknowledgeMessage(int socketId, struct sockaddr_in destinationAddress, socklen_t destinationAddressLength);

/*
    Rotina para envio de uma mensagem, baseada no sistema de ACKs do protocolo TCP
    É dividida em 2 partes: o envio da mensagem e o recebimento do ACK que confirma que a mensagem foi recebida
    Primeiro, a rotina envia a mensagem, então fica em busy waiting esperando receber o ACK
    Se não receber até TIMEOUT_SECONDS segundos, reenvia a mensagem    
*/
void* SendMessageInSocket(void* socketIdAndMessageData);

#endif