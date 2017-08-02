// HATRED.cpp : Defines the entry point for the console application.
//

// TODO: 
// 1) avoid gui communication when pairing devices
// 2) change tcp to udp

#include "stdafx.h"
#include <iostream>
#include "BluetoothAdapter.h"
#include "ServerRX.h"
#include "ServerTX.h"

int main(int argc, char **args)
{
    BluetoothAdapter adapter;

    if (adapter.pairDevice(L"5CG4383LR5"))
    {
        ServerTX server(adapter.getMacAddressOfConnectedDevice());
        server.run();
    }

    

    printf("going to exit\n");
    getchar();

    return 0;

}