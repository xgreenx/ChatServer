
#include "Server.h"

#include <err.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

using namespace std;

event_base* Server::baseEvent = event_base_new();
set<Client*> Server::clients = set<Client*>();
int Server::counter = 0;
map<string, string> Server::users;

void informAll(const char* data, size_t size, Client* currentClient = NULL)
{
    for (auto& client : Server::clients)
    {
        if (client != currentClient)
        {
            bufferevent_write(client->bufEvent, data, size);
        }
    }
}

bool changeNickname(Client* user, const string& name)
{
    if (name.size() <= 1)
    {
        string message = "Your a new nickname '" + name + "' must contains more valid symbols(>= 2)\n";
        bufferevent_write(user->bufEvent, message.data(), message.size());
        return false;
    }

    if (Server::users.count(name))
    {
        string message = "This nickname is locked\n";
        bufferevent_write(user->bufEvent, message.data(), message.size());
        return false;
    }

    for (auto& client : Server::clients)
    {
        if (client->name == name)
        {
            string message = "This nickname is used by another user\n";
            bufferevent_write(user->bufEvent, message.data(), message.size());
            return false;
        }
    }

    string message = user->name + " changed his nickname to '" + name + "'\n";

    user->name = name;
    informAll(message.data(), message.size());

    return true;
}

bool processCommand(Client* user, const string& command, char* data = NULL)
{
    if (command == "list")
    {
        string message;
        for (auto& client : Server::clients)
        {
            message += client->name + "\n";
        }

        bufferevent_write(user->bufEvent, message.data(), message.size());
        return true;
    }
    else if (command == "nickname")
    {
        while(isspace(*data))
        {
            ++data;
        }

        string nickname;

        while(isgraph(*data))
        {
            nickname += *data;
            ++data;
        }

        return changeNickname(user, nickname);
    }
    else if (command == "registr")
    {
        while(isspace(*data))
        {
            ++data;
        }

        string password;

        while(isgraph(*data))
        {
            password += *data;
            ++data;
        }

        if (password.empty())
        {
            string message = "Password can't be empty\n";
            bufferevent_write(user->bufEvent, message.data(), message.size());
            return false;
        }

        Server::users[user->name] = password;

        string message = "Your nickname " + user->name + " now is locked by password '" + password + "'\n";
        bufferevent_write(user->bufEvent, message.data(), message.size());
        return true;
    }
    else if (command == "exit")
    {
        Server::onClientError(NULL, BEV_EVENT_EOF, user);
        return true;
    }

    return false;
}

void Server::init(int port)
{
    int MAX_CONNECTIONS = 10;
    int listenSocket;
    struct sockaddr_in listenAddr;
    struct event evAccept;

    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0)
    {
        err(1, "listen failed");
    }

    memset(&listenAddr, 0, sizeof(listenAddr));
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_addr.s_addr = INADDR_ANY;
    listenAddr.sin_port = htons(port);

    if (bind(listenSocket, (struct sockaddr *)&listenAddr, sizeof(listenAddr)) < 0)
    {
        err(1, "bind failed");
    }

    if (listen(listenSocket, MAX_CONNECTIONS) < 0)
    {
        err(1, "listen failed");
    }
    int temp = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(temp));

    if (Server::setNonBlock(listenSocket) < 0)
    {
        err(1, "failed to set server socket to non-blocking");
    }

    event_assign(&evAccept, Server::baseEvent, listenSocket, EV_READ|EV_PERSIST, Server::onClientAccept, NULL);
    event_add(&evAccept, NULL);

    event_base_dispatch(Server::baseEvent);
}

int Server::setNonBlock(int socket)
{
    int flags;

    flags = fcntl(socket, F_GETFL);

    if (flags < 0)
    {
        return flags;
    }

    flags |= O_NONBLOCK;

    if (fcntl(socket, F_SETFL, flags) < 0)
    {
        return -1;
    }

    return 0;
}

void Server::onClientAccept(int socket, short ev, void* arg)
{
    int clientSocket;
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    clientSocket = accept(socket, (struct sockaddr *)&clientAddr, &clientLen);
    if (clientSocket < 0)
    {
        warn("accept failed");
        return;
    }

    if (Server::setNonBlock(clientSocket) < 0)
    {
        warn("failed to set client socket non-blocking");
    }

    Client* client = new Client();

    client->socket = clientSocket;
    client->bufEvent = bufferevent_socket_new(baseEvent, clientSocket, 0);

    string name = "Guest" + to_string(Server::counter++);
    while (Server::users.count(name))
    {
        name = "Guest" + to_string(Server::counter++);
    }

    client->name = name;
    bufferevent_setcb(client->bufEvent, onClientRead, NULL, &onClientError, client);

    clients.insert(client);

    bufferevent_enable(client->bufEvent, EV_READ);

    printf("Accepted connection from %s\n", inet_ntoa(clientAddr.sin_addr));
    string message = client->name + " is connected\n";
    informAll(message.data(), message.size());
}

void Server::onClientRead(struct bufferevent *bev, void *arg)
{
    Client* currentClient = (Client*)arg;
    size_t MAX_MESSAGE_SIZE = 8192;
    char data[MAX_MESSAGE_SIZE] = {0};

    int i = 0;
    while(++i)
    {
        size_t offset = 0;
        if (i == 1)
        {
            string prefix = currentClient->name + ": ";
            strcpy(data, prefix.data());
            offset = prefix.size();
        }

        size_t n = bufferevent_read(bev, data + offset, sizeof(data) - offset);

        if (data[offset] == '/')
        {
            cout << "Process command ";
            ++offset;
            string command;
            while(isalpha(data[offset]))
            {
                command += data[offset++];
            }

            cout << "'" << command << "'" << endl;

            if (processCommand(currentClient, command, data + offset))
            {
                return;
            }
        }

        if (n <= 0)
        {
            break;
        }

        if (i == 1)
        {
            n += offset;
        }

        if (n < MAX_MESSAGE_SIZE)
        {
            data[n] = '\n';
        }

        informAll(data, n, currentClient);
    }
}

void Server::onClientError(struct bufferevent *bev, short what, void *arg)
{
    Client* currentClient = (Client*) arg;

    if (what & BEV_EVENT_EOF)
    {
        printf("Client disconnected.\n");
        string message = currentClient->name + " is disconnected\n";
        informAll(message.data(), message.size(), currentClient);
    }
    else
    {
        warn("Client socket error, disconnecting.\n");
    }

    clients.erase(currentClient);

    bufferevent_free(currentClient->bufEvent);
    close(currentClient->socket);
    delete(currentClient);
}
