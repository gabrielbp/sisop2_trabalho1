#include "include/sockets/sockets.h"
#include <vector>
#include <functional>
/*
    Classe que define eventos
*/
class MessageEventsSource {
public:

    std::vector <std::pair<std::function<void(T_SocketMessageData)>, int>> receivedNewMessageSubscribedCallbacks;
    std::vector <std::function<void(T_SocketMessageData)>> receivedNewConnectionSubscribedCallbacks;

    void PublishReceivedNewMessage(T_SocketMessageData messageData, int socketId){   //publicado sempre que chega um datagrama em algum socket que não seja o connect
        int i = 0;
        for(i = 0; i<receivedNewMessageSubscribedCallbacks.size(); i++){
            if(receivedNewMessageSubscribedCallbacks[i].second == socketId)
                receivedNewMessageSubscribedCallbacks[i].first(messageData);
        }
    }

    int SubscribeToReceivedNewMessage(std::function<void(T_SocketMessageData)> foo, int socketId){
        receivedNewMessageSubscribedCallbacks.push_back(std::pair<std::function<void(T_SocketMessageData)>, int>(foo, socketId));
        return (receivedNewMessageSubscribedCallbacks.size() -1);
    }

    void UnsubscribeToReceivedNewMessage(int index){
        receivedNewMessageSubscribedCallbacks.erase(receivedNewMessageSubscribedCallbacks.begin()+index);
    }


    void PublishReceivedNewConnection(T_SocketMessageData messageData){   //publicado quando chega um novo datagrama no socket connect
        int i = 0;
        for(i = 0; i<receivedNewConnectionSubscribedCallbacks.size(); i++){
            receivedNewConnectionSubscribedCallbacks[i](messageData);
        }
    }

    void SubscribeToReceivedNewConnection(std::function<void(T_SocketMessageData)> foo){
        receivedNewConnectionSubscribedCallbacks.push_back(foo);
    }

    void UnsubscribeToReceivedNewConnection(std::function<void(T_SocketMessageData)> foo){
        int i = 0;
        for(i = 0; i<receivedNewMessageSubscribedCallbacks.size(); i++){
            receivedNewConnectionSubscribedCallbacks.erase(receivedNewConnectionSubscribedCallbacks.begin() + i);
        }
    }
};