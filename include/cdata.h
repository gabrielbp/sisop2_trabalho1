#ifndef __cdata__
#define __cdata__

#include <string>

#define MAX_DEVICES 2

struct Device {
    int id;
    int socket1;
    int socket2;
    int socket3;
};

struct User {
    string name;
    Device devices[MAX_DEVICES];
};

typedef struct packet{
    uint16_t type;               //Tipo do pacote(p.ex. DATA| ACK | CMD)
    uint16_t seqn;               //Número de sequência
    uint32_t total_size;         //Número total de fragmentos
    uint16_t length;             //Comprimento do payload 
    const char* _payload;        //Dados do pacote
} packet;

typedef struct commandPacket {
    uint64_t packetType;
    uint64_t command;
    char additionalInfo[100];
} commandPacket;

#endif