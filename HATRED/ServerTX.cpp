#include "stdafx.h"
#include "ServerTX.h"

#include <stdio.h>
#include <initguid.h>
#include <winsock2.h>
#include <ws2bth.h>
#include <strsafe.h>
#include <intsafe.h>

const LPWSTR MOJ_ASUS = L"DC:53:60:6F:E3:1A";
const LPWSTR NOKIA    = L"80:00:0B:24:10:25";
const LPWSTR ELEPHONE = L"94:71:BC:89:A8:AB";

ServerTX::ServerTX(std::string &remoteMacAddress)
{
    // Now you know, why project is called HATRED?
    size_t size = strlen(remoteMacAddress.c_str()) + 1;
    mRemoteMacAddress = new wchar_t[size];

    size_t outSize;
    mbstowcs_s(&outSize, mRemoteMacAddress, size, remoteMacAddress.c_str(), size - 1);
}


ServerTX::~ServerTX()
{
    delete[] mRemoteMacAddress;
}

bool ServerTX::openConnection()
{
    SOCKADDR_BTH remoteBthAddr = { 0 };

    int addressLength = sizeof(SOCKADDR_BTH);
    int result = WSAStringToAddress(mRemoteMacAddress, AF_BTH, NULL, (LPSOCKADDR)&remoteBthAddr, &addressLength);
    if (result != 0)
    {
        wprintf(L"=CRITICAL= | WSAStringToAddressW call failed. WSAGetLastError = [%d]\n", WSAGetLastError());
        return false;
    }

    // Setting address family to AF_BTH indicates winsock2 to use Bluetooth sockets
    // Port should be set to 0 if ServiceClassId is specified.
    remoteBthAddr.addressFamily = AF_BTH;
    remoteBthAddr.serviceClassId = getGUIDSpecifier();
    remoteBthAddr.port = 0;

    // Open a bluetooth socket using RFCOMM protocol
    mLocalSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    if (INVALID_SOCKET == mLocalSocket) 
    {
        wprintf(L"=CRITICAL= | socket() call failed. WSAGetLastError = [%d]\n", WSAGetLastError());
        return false;
    }

    // Connect the socket to a given remote socket represented by address
    result = connect(mLocalSocket, (struct sockaddr *) &remoteBthAddr, sizeof(SOCKADDR_BTH));
    if (SOCKET_ERROR == result) 
    {
        wprintf(L"=CRITICAL= | connect() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
        return false;
    }

    return true;
}

bool ServerTX::sendData()
{   
#define CXN_TEST_DATA_STRING              L"~!@#$%^&*()-_=+?<>1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define CXN_TRANSFER_DATA_LENGTH          (sizeof(CXN_TEST_DATA_STRING))

    wprintf(L"*INFO* | Sending following data string:\n%s\n", CXN_TEST_DATA_STRING);

    int result = send(mLocalSocket, (const char *)CXN_TEST_DATA_STRING, (int)CXN_TRANSFER_DATA_LENGTH, 0);

    if (SOCKET_ERROR == result) 
    {
        wprintf(L"=CRITICAL= | send() call failed w/socket = [0x%I64X], dataLen = [%I64u]. WSAGetLastError=[%d]\n", (ULONG64)mLocalSocket, (ULONG64)CXN_TRANSFER_DATA_LENGTH, WSAGetLastError());        
        return false;
    }

    return true;
}

void ServerTX::run()
{
    if (initSocks() && openConnection())
    {
        sendData();
    }
}

