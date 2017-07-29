#include "stdafx.h"
#include "BluetoothAdapter.h"

BluetoothAdapter::BluetoothAdapter() : mRadioEnumerator(nullptr), mRadio(nullptr), mDeviceEnumerator(nullptr)
{
    ZeroMemory(&mDeviceInfo, sizeof(mDeviceInfo));
}


BluetoothAdapter::~BluetoothAdapter()
{
    if (!BluetoothFindRadioClose(mRadioEnumerator))
    {
        printf("~BluetoothAdapter: BluetoothFindRadioClose failed with error code %d\n", GetLastError());
    }

    if (!BluetoothFindDeviceClose(mDeviceEnumerator))
    {
        printf("~BluetoothAdapter: BluetoothFindDeviceClose failed with error code %d\n", GetLastError());
    }
}

void BluetoothAdapter::dumpRadioInfo()
{
    BLUETOOTH_RADIO_INFO radioInfo = { sizeof(radioInfo), 0, };

    DWORD result = BluetoothGetRadioInfo(mRadio, &radioInfo);

    if (result != ERROR_SUCCESS)
    {
        printf("BluetoothAdapter::dumpRadioInfo: failed with error code %d\n", result);
        return;
    }


    wprintf(L"Found Radio\n");
    wprintf(L"\tInstance Name: %s\n", radioInfo.szName);

    wprintf(L"\tAddress: %02X:%02X:%02X:%02X:%02X:%02X\n", radioInfo.address.rgBytes[5],
        radioInfo.address.rgBytes[4], radioInfo.address.rgBytes[3], radioInfo.address.rgBytes[2],
        radioInfo.address.rgBytes[1], radioInfo.address.rgBytes[0]);

    wprintf(L"\tClass: 0x%08x\r\n", radioInfo.ulClassofDevice);
    wprintf(L"\tManufacturer: 0x%04x\r\n", radioInfo.manufacturer);
    wprintf(L"-----------------------------------------------------------------------\n");
}

void BluetoothAdapter::dumpDeviceInfo(const BLUETOOTH_DEVICE_INFO &deviceInfo)
{   
    wprintf(L"Found Device\n");
    wprintf(L"  \tInstance Name: %s\r\n", deviceInfo.szName);
    wprintf(L"  \tAddress: %02X:%02X:%02X:%02X:%02X:%02X\r\n", deviceInfo.Address.rgBytes[5],
        deviceInfo.Address.rgBytes[4], deviceInfo.Address.rgBytes[3], deviceInfo.Address.rgBytes[2],
        deviceInfo.Address.rgBytes[1], deviceInfo.Address.rgBytes[0]);

    wprintf(L"  \tClass: 0x%08x\r\n", deviceInfo.ulClassofDevice);
    wprintf(L"  \tConnected: %s\r\n", deviceInfo.fConnected ? L"true" : L"false");
    wprintf(L"  \tAuthenticated: %s\r\n", deviceInfo.fAuthenticated ? L"true" : L"false");
    wprintf(L"  \tRemembered: %s\r\n", deviceInfo.fRemembered ? L"true" : L"false");
    wprintf(L"-----------------------------------------------------------------------\n");
}

bool BluetoothAdapter::pairDevice(const WCHAR *name)
{
    wprintf(L"BluetoothAdapter::pairDevice: Going to pair a device '%s'\n", name);

    if (findFirstRadio())
    {
        for (int i = 0; i < 10; i++)
        {
            if (findDevice(name))
            {
                if (authenticate())
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool BluetoothAdapter::findFirstRadio()
{
    if (mRadioEnumerator != nullptr)
    {
        return true;
    }

    printf("BluetoothAdapter::findFirstRadio: Looking for bluetooth radio....\n");

    BLUETOOTH_FIND_RADIO_PARAMS findRadioParams = { sizeof(findRadioParams) };

    mRadioEnumerator = BluetoothFindFirstRadio(&findRadioParams, &mRadio);

    if (mRadioEnumerator == nullptr)
    {
        printf("BluetoothAdapter::findFirstRadio: failed with error code %d\n", GetLastError());
        return false;
    }

    dumpRadioInfo();

    return true;
}

bool BluetoothAdapter::findDevice(const WCHAR *name)
{
    wprintf(L"BluetoothAdapter::findDevice: Looking for bluetooth device '%s'....\n", name);

    BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams = 
    {
        sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS), //  IN  sizeof this structure
        1,                                      //  IN  return authenticated devices
        1,                                      //  IN  return remembered devices
        1,                                      //  IN  return unknown devices
        1,                                      //  IN  return connected devices
        1,                                      //  IN  issue a new inquiry
        3,                                      //  IN  timeout for the inquiry
        mRadio                                  //  IN  handle to radio to enumerate - NULL == all radios will be searched
    };


    BLUETOOTH_DEVICE_INFO deviceInfo = { sizeof(deviceInfo), 0, };

    mDeviceEnumerator = BluetoothFindFirstDevice(&searchParams, &deviceInfo);

    if (mDeviceEnumerator == nullptr)
    {
        printf("BluetoothAdapter::findDevice: failed with error code %d\n", GetLastError());
        return false;
    }

    do
    {
        dumpDeviceInfo(deviceInfo);

        if (wcscmp(name, deviceInfo.szName) == 0)
        {
            wprintf(L"BluetoothAdapter::findDevice: Found device with name '%s'!\n", name);

            mDeviceInfo = deviceInfo;

            return true;
        }
    } while (BluetoothFindNextDevice(mDeviceEnumerator, &deviceInfo));

    wprintf(L"BluetoothAdapter::findDevice: Unable to find device with name '%s'!\n", name);

    if (!BluetoothFindDeviceClose(mDeviceEnumerator))
    {
        printf("BluetoothAdapter::findDevice: BluetoothFindDeviceClose failed with error code %d\n", GetLastError());
    }

    return false;
}

bool BluetoothAdapter::authenticate()
{
    if (!mDeviceInfo.fAuthenticated)
    {
        DWORD result = BluetoothAuthenticateDeviceEx(nullptr, mRadio, &mDeviceInfo, nullptr, MITMProtectionRequiredGeneralBonding);

        if (result != ERROR_SUCCESS)
        {
            printf("BluetoothAdapter::authenticate: failed with error code %d\n", result);
            return false;
        }
    }

    printf("BluetoothAdapter::authenticate: Successfully authenticated!\n");
    return true;
    
}
