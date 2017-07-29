// HATRED.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "BluetoothAdapter.h"

int main(int argc, char **args)
{
    BluetoothAdapter adapter;

    adapter.pairDevice(L"5CG4383LR5");

    adapter.runServerMode();

    printf("going to exit\n");
    getchar();

    return 0;

}