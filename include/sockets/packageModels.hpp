#define MAX_PAYLOAD_SIZE 1024

enum class MessageType{
    ACK,
    DEFINE_PORT,
    ETC
};

typedef struct message{
    MessageType type;
    int packageOrder;
    char payload[MAX_PAYLOAD_SIZE];
} T_Message;