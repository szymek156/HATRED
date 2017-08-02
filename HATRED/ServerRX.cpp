#include "stdafx.h"
#include "ServerRX.h"

#include <stdio.h>
#include <initguid.h>
#include <winsock2.h>
#include <ws2bth.h>
#include <strsafe.h>
#include <intsafe.h>
#include <memory>

ServerRX::ServerRX() : mClientSocket(INVALID_SOCKET)
{
}


ServerRX::~ServerRX()
{   
    closesocket(mClientSocket);
}


bool ServerRX::openConnection()
{
    // Open a bluetooth socket using RFCOMM protocol
    mLocalSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);

    if (INVALID_SOCKET == mLocalSocket)
    {
        wprintf(L"=CRITICAL= | socket() call failed. WSAGetLastError = [%d]\n", WSAGetLastError());
    }

    SOCKADDR_BTH sockAddrLocal = { 0 };
    // Setting address family to AF_BTH indicates winsock2 to use Bluetooth port        
    sockAddrLocal.addressFamily = AF_BTH;
    sockAddrLocal.port = BT_PORT_ANY;

    //
    // bind() associates a local address and port combination
    // with the socket just created. This is most useful when
    // the application is a server that has a well-known port
    // that clients know about in advance.
    //
    if (SOCKET_ERROR == bind(mLocalSocket, (struct sockaddr *) &sockAddrLocal, sizeof(sockAddrLocal)))
    {
        wprintf(L"=CRITICAL= | bind() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", (ULONG64)mLocalSocket, WSAGetLastError()); \
    }

    int iAddrLen = sizeof(sockAddrLocal);
    if (SOCKET_ERROR == getsockname(mLocalSocket, (struct sockaddr *)&sockAddrLocal, &iAddrLen))
    {
        wprintf(L"=CRITICAL= | getsockname() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", (ULONG64)mLocalSocket, WSAGetLastError());
    }


    // If we got an address, go ahead and advertise it.
    WSAQUERYSET wsaQuerySet = { 0 };

    CSADDR_INFO addressInfo = { 0 };

    addressInfo.LocalAddr.iSockaddrLength = sizeof(sockAddrLocal);
    addressInfo.LocalAddr.lpSockaddr = (LPSOCKADDR)&sockAddrLocal;
    addressInfo.RemoteAddr.iSockaddrLength = sizeof(sockAddrLocal);
    addressInfo.RemoteAddr.lpSockaddr = (LPSOCKADDR)&sockAddrLocal;
    addressInfo.iSocketType = SOCK_STREAM;
    addressInfo.iProtocol = BTHPROTO_RFCOMM;

    wsaQuerySet.dwSize = sizeof(WSAQUERYSET);
    wsaQuerySet.lpServiceClassId = &getGUIDSpecifier();
    wsaQuerySet.lpszServiceInstanceName = createInstanceName();
    wsaQuerySet.lpszComment = L"Example Service instance registered in the directory service through RnR";
    wsaQuerySet.dwNameSpace = NS_BTH;
    wsaQuerySet.dwNumberOfCsAddrs = 1;      // Must be 1.
    wsaQuerySet.lpcsaBuffer = &addressInfo; // Req'd.

    //
    // As long as we use a blocking accept(), we will have a race
    // between advertising the service and actually being ready to
    // accept connections.  If we use non-blocking accept, advertise
    // the service after accept has been called.
    //
    if (SOCKET_ERROR == WSASetService(&wsaQuerySet, RNRSERVICE_REGISTER, 0))
    {
        wprintf(L"=CRITICAL= | WSASetService() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
    }

    wprintf(L"Server::openConnection: Success.\n");
    return true;
}

bool ServerRX::waitClientToConnect()
{
    wprintf(L"Server::waitClientToConnect: Waiting...\n");

    // listen() call indicates winsock2 to listen on a given socket for any incoming connection.    
    int queueSize = 4;
    if (SOCKET_ERROR == listen(mLocalSocket, queueSize))
    {
        wprintf(L"Server::waitClientToConnect listen() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", (ULONG64)mLocalSocket, WSAGetLastError());
        return false;
    }

    // accept() call indicates winsock2 to wait for any
    // incoming connection request from a remote socket.
    // If there are already some connection requests on the queue,
    // then accept() extracts the first request and creates a new socket and
    // returns the handle to this newly created socket. This newly created
    // socket represents the actual connection that connects the two sockets.
    mClientSocket = accept(mLocalSocket, NULL, NULL);

    if (INVALID_SOCKET == mClientSocket)
    {
        wprintf(L"Server::waitClientToConnect: accept() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
        return false;
    }

    wprintf(L"Server::waitClientToConnect: Client connected.\n");

    return true;
}

void ServerRX::readData()
{
    wprintf(L"Server::readData: Waiting for data...\n");

#define CXN_TEST_DATA_STRING              (L"~!@#$%^&*()-_=+?<>1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
#define CXN_TRANSFER_DATA_LENGTH          (sizeof(CXN_TEST_DATA_STRING))
    // Read data from the incoming stream
    char *buffer = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CXN_TRANSFER_DATA_LENGTH);
    char *offset = buffer;

    bool readMore = true;
    int totalReceived = 0;

    while (readMore && (totalReceived < CXN_TRANSFER_DATA_LENGTH))
    {
        //
        // recv() call indicates winsock2 to receive data
        // of an expected length over a given connection.
        // recv() may not be able to get the entire length
        // of data at once.  In such case the return value,
        // which specifies the number of bytes received,
        // can be used to calculate how much more data is
        // pending and accordingly recv() can be called again.
        //
        int received = recv(mClientSocket, (char *)offset, (CXN_TRANSFER_DATA_LENGTH - totalReceived), 0);

        switch (received)
        {
        case 0:
        {// socket connection has been closed gracefully
            readMore = false;
            break;
        }
        case SOCKET_ERROR:
        {
            wprintf(L"=CRITICAL= | recv() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
            readMore = false;
            break;
        }
        default:
        {
            offset += received;
            totalReceived += received;
            break;
        }
        }
    }

    if (CXN_TRANSFER_DATA_LENGTH != totalReceived)
    {
        wprintf(L"+WARNING+ | Data transfer aborted mid-stream. Expected Length = [%I64u], Actual Length = [%d]\n", (ULONG64)CXN_TRANSFER_DATA_LENGTH, totalReceived);
    }
    wprintf(L"*INFO* | Received following data string from remote device:\n%s\n", (wchar_t *)buffer);
}


void ServerRX::run()
{
    if (initSocks() && openConnection() && waitClientToConnect())
    {
        readData();
    }
}


wchar_t *ServerRX::createInstanceName()
{
    wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
    DWORD   computerNameLength = MAX_COMPUTERNAME_LENGTH + 1;

    GetComputerName(computerName, &computerNameLength);

    size_t size = 0;
    StringCchLength(computerName, sizeof(computerName), &size);

    wchar_t *instanceName = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    StringCbPrintf(instanceName, size, L"%s_HATRED", computerName);

    return instanceName;
}