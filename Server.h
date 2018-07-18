//
// Created by green on 17.07.18.
//

#ifndef UNTITLED_SERVER_H
#define UNTITLED_SERVER_H

#include "Client.h"
#include <set>
#include <map>

using namespace std;

class Server
{
public:
    static struct event_base *baseEvent;
    static set<Client*> clients;
    static int counter;
    static map<string, string> users;

    void static init(int port = 1488);
    int static setNonBlock(int socket);

    void static onClientAccept(int socket, short ev, void* arg);

    void static onClientRead(struct bufferevent *bev, void *arg);
    void static onClientError(struct bufferevent *bev, short what, void *arg);
};


#endif //UNTITLED_SERVER_H
