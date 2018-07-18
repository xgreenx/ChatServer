#include <iostream>
#include "Server.h"

using namespace std;

int main(int argc, char *argv[])
{
    int port = 2283;

    if (argc < 2)
    {
        cout << "I will use default port " << to_string(port) << endl;
    }
    else
    {
        port = stoi(argv[1]);
        cout << "Use port " << to_string(port) << endl;
    }

    Server::init(port);

    return 0;
}