#include "stdafx.h"
#include "Server.h"
#include <winsock2.h>
#include <initguid.h>

// {B62C4E8D-62CC-404b-BBBF-BF3E3BBB1374}
DEFINE_GUID(g_guidServiceClass, 0xb62c4e8d, 0x62cc, 0x404b, 0xbb, 0xbf, 0xbf, 0x3e, 0x3b, 0xbb, 0x13, 0x74);

Server::Server() : mLocalSocket(INVALID_SOCKET)
{
}

Server::~Server()
{
    closesocket(mLocalSocket);
}

bool Server::initSocks()
{
    // Ask for Winsock version 2.2.
    WSADATA data = { 0 };
    ULONG status = WSAStartup(MAKEWORD(2, 2), &data);

    if (0 != status)
    {
        wprintf(L"Server::initSocks: Unable to initialize Winsock version 2.2, status %d\n", status);
        return false;
    }

    return true;
}

GUID Server::getGUIDSpecifier()
{
    return g_guidServiceClass;
}