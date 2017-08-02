#pragma once
#include "Server.h"
#include <string>

class ServerTX : public Server
{
public:
    ServerTX(std::string &remoteMacAddress);
    ~ServerTX();

    void run() override;

protected:
    bool openConnection();
    bool sendData();

    wchar_t *mRemoteMacAddress;
};

