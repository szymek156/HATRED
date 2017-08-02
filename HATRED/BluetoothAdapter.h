#pragma once
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <Winsock2.h>
#include <Ws2bth.h>
#include <BluetoothAPIs.h>
#include <string>

class BluetoothAdapter
{
public:
    BluetoothAdapter();
    virtual ~BluetoothAdapter();

    bool pairDevice(const WCHAR *name);
    std::string getMacAddressOfConnectedDevice();

protected:
    bool findFirstRadio();
    bool findDevice(const WCHAR *name);
    bool authenticate();
    void dumpRadioInfo();
    void dumpDeviceInfo(const BLUETOOTH_DEVICE_INFO &deviceInfo);

    HANDLE mRadio;
    HBLUETOOTH_RADIO_FIND mRadioEnumerator;
    HBLUETOOTH_DEVICE_FIND mDeviceEnumerator;
    BLUETOOTH_DEVICE_INFO mDeviceInfo;
};

