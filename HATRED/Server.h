#pragma once
#include <winsock2.h>

class Server
{
public:
    Server();
    virtual ~Server();
    virtual void run() = 0;
protected:
    bool initSocks();
    static GUID getGUIDSpecifier();

    SOCKET mLocalSocket;
};

