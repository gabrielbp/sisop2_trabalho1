#ifndef SOCKETS
#define SOCKETS

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <chrono>
#include <fcntl.h>
#include "../threads/threads.h"
#include "packageModels.hpp"

#define NUM_MAX_CONNECTIONS 256
#define DEFAULT_OPTIONS 0
#define SOCKET_BUFFER_LEN 1024
#define TIMEOUT_SECONDS 5.0
#define ANY_PORT 0

/*
    Estrutura de dados que representa um socket conectado a um dispositivo remoto
*/
typedef struct SocketConnection{
    int isValid;                            //Valor booleano utilizado para identificar sockets que já foram excluídos
    int socketId;                           //Identificador do socket vinculado a esta conexão
    struct sockaddr_in connectionAddress;   //endereço de destino desta conexão
    socklen_t connectionAddressLength;      //tamanho do endereço de destino
    char* username;                         //nome do usuário vinculado a esta conexão
    int threadId;                           //Identificador da thread correspondente a esta conexão
} T_SocketConnection;

/*
    Uma estrutura que contém os dados referentes a uma mensagem recebida/enviada no socket
*/
typedef struct socketMessageData{
    T_Message message;                   //a mensagem em si
    struct sockaddr_in senderAddress;   //o endereço do remetente
    socklen_t senderAddressLength;      //o tamanho da estrutura do endereço do remetente
    int socketId;                       //socket no qual foi recebida a mensagem
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
    T_SocketMessageData messageData;
} T_SendMessageInSocketParameter;

/*
    Cria um novo socket de cliente
    Retorna o identificador do socket se executou com sucesso ou -1 se houve algum erro
*/
int NewClientSocket();


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
    Dados o hostname, o nome de usuário, um identificador de socket inicializado, uma porta e uma função callback, executa o procedimento de conexão
    com o servidor e inicia uma thread que ficará lendo no socket.
    Retorna o número da porta que deverá ser usada pelas próximas requisições ao servidor se executou com sucesso
    Se houve erro, retorna -1
*/
int Connect(char* hostname, char* username, int socketId, int port, void* (*messageReceivedCallback)(T_SocketMessageData));

/*
    Dados um nome de host, uma porta e um buffer para a estrutura de endereço de socket,
    preenche o buffer com as informações do servidor, se encontrar
    Retorna 0 se executou com sucesso e -1 se houve algum erro
*/
int GetServerAddress(char *hostname, int port, struct sockaddr_in *serverAddress);

/*
    Dada uma porta, cria um novo socket de servidor vinculado a esta porta.
    Retorna o identificador deste socket se executou com sucesso, -1 se houve um erro
*/
int NewServerSocket(int port);

/*
    Dado um identificador de socket, retorna a estrutura referente ao endereço deste socket
    ou -1 se houver algum erro
*/
struct sockaddr_in GetSocketAddress(int socketId);

#endif