#pragma once
#include <winsock2.h>
#include "Server.h"

class ServerRX : public Server
{
public:
    ServerRX();
    virtual ~ServerRX();

    virtual void run() override;

protected:
    bool openConnection();
    bool waitClientToConnect();
    void readData();

    wchar_t *createInstanceName();

    SOCKET mClientSocket;
};

