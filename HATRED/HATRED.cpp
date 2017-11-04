// HATRED.cpp : Defines the entry point for the console application.
//

// TODO: 
// 1) avoid gui communication when pairing devices
// 2) change tcp to udp
// 3) there is an temptetion to implement deque, ring buffer using atomics

#include "stdafx.h"
#include <iostream>
#include "BluetoothAdapter.h"
#include "ServerRX.h"
#include "ServerTX.h"
#include "AudioCapture.h"
#include "Encoder.h"
#include "Container.h"
#include <thread>

void startServerTX(std::string &macAddress)
{
    ServerTX server(macAddress);
    server.run();
}

void startServerRX()
{
    ServerRX server;
    server.run();
}

int main(int argc, char **args)
{
    //BluetoothAdapter adapter;

   // if (adapter.pairDevice(L"5CG4383LR5"))
    {
        AudioCapture audio;
        audio.run();
        /*std::thread rx(startServerRX);
        std::thread tx(startServerTX, adapter.getMacAddressOfConnectedDevice());
        

        rx.join();
        tx.join();*/
    }

    printf("going to exit\n");
    getchar();

    return 0;

}