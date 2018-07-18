//
// Created by green on 17.07.18.
//

#ifndef UNTITLED_CLIENT_H
#define UNTITLED_CLIENT_H

using namespace std;

#include <string>

class Client {
public:
    int socket;
    struct bufferevent *bufEvent;

    string name;
    string password;
};


#endif //UNTITLED_CLIENT_H
