// HATRED.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "BluetoothAdapter.h"
#include "Server.h"

int main(int argc, char **args)
{
    BluetoothAdapter adapter;

    if (adapter.pairDevice(L"5CG4383LR5"))
    {
        Server server;
        server.run();
    }

    

    printf("going to exit\n");
    getchar();

    return 0;

}