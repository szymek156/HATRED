#pragma once
#include <winsock2.h>

class Server
{
public:
    Server();
    ~Server();
    void run();

protected:
    bool initSocks();
    bool openConnection();
    bool waitClientToConnect();
    void readData();

    wchar_t *createInstanceName();

    SOCKET mLocalSocket;
    SOCKET mClientSocket;
};

