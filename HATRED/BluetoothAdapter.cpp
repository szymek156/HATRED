#include "stdafx.h"
#include "BluetoothAdapter.h"

#include <stdio.h>
#include <initguid.h>
#include <winsock2.h>
#include <ws2bth.h>
#include <strsafe.h>
#include <intsafe.h>

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

#define CXN_TEST_DATA_STRING              (L"~!@#$%^&*()-_=+?<>1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
#define CXN_TRANSFER_DATA_LENGTH          (sizeof(CXN_TEST_DATA_STRING))


#define CXN_BDADDR_STR_LEN                17   // 6 two-digit hex values plus 5 colons
#define CXN_MAX_INQUIRY_RETRY             3
#define CXN_DELAY_NEXT_INQUIRY            15
#define CXN_SUCCESS                       0
#define CXN_ERROR                         1
#define CXN_DEFAULT_LISTEN_BACKLOG        4
// {B62C4E8D-62CC-404b-BBBF-BF3E3BBB1374}
DEFINE_GUID(g_guidServiceClass, 0xb62c4e8d, 0x62cc, 0x404b, 0xbb, 0xbf, 0xbf, 0x3e, 0x3b, 0xbb, 0x13, 0x74);

void BluetoothAdapter::runServerMode()
{
    //
    // Ask for Winsock version 2.2.
    //
    WSADATA     WSAData = { 0 };
    ULONG ulRetCode = WSAStartup(MAKEWORD(2, 2), &WSAData);

    if (CXN_SUCCESS != ulRetCode) {
        wprintf(L"-FATAL- | Unable to initialize Winsock version 2.2\n");
    }

    //
    // RunServerMode runs the application in server mode.  It opens a socket, connects it to a
    // remote socket, transfer some data over the connection and closes the connection.
    //

    #define CXN_INSTANCE_STRING L"Sample Bluetooth Server"
    int iMaxCxnCycles = 0;
    
    int             iAddrLen = sizeof(SOCKADDR_BTH);
    int             iCxnCount = 0;
    UINT            iLengthReceived = 0;
    UINT            uiTotalLengthReceived;
    size_t          cbInstanceNameSize = 0;
    char *          pszDataBuffer = NULL;
    char *          pszDataBufferIndex = NULL;
    wchar_t *       pszInstanceName = NULL;
    wchar_t         szThisComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD           dwLenComputerName = MAX_COMPUTERNAME_LENGTH + 1;
    SOCKET          LocalSocket = INVALID_SOCKET;
    SOCKET          ClientSocket = INVALID_SOCKET;
    WSAQUERYSET     wsaQuerySet = { 0 };
    SOCKADDR_BTH    SockAddrBthLocal = { 0 };
    LPCSADDR_INFO   lpCSAddrInfo = NULL;
    HRESULT         res;

    //
    // This fixed-size allocation can be on the stack assuming the
    // total doesn't cause a stack overflow (depends on your compiler settings)
    // However, they are shown here as dynamic to allow for easier expansion
    //
    lpCSAddrInfo = (LPCSADDR_INFO)HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        sizeof(CSADDR_INFO));
    if (NULL == lpCSAddrInfo) {
        wprintf(L"!ERROR! | Unable to allocate memory for CSADDR_INFO\n");
        ulRetCode = CXN_ERROR;
    }

    if (CXN_SUCCESS == ulRetCode) {

        if (!GetComputerName(szThisComputerName, &dwLenComputerName)) {
            wprintf(L"=CRITICAL= | GetComputerName() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
            ulRetCode = CXN_ERROR;
        }
    }

    //
    // Open a bluetooth socket using RFCOMM protocol
    //
    if (CXN_SUCCESS == ulRetCode) {
        LocalSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
        if (INVALID_SOCKET == LocalSocket) {
            wprintf(L"=CRITICAL= | socket() call failed. WSAGetLastError = [%d]\n", WSAGetLastError());
            ulRetCode = CXN_ERROR;
        }
    }

    if (CXN_SUCCESS == ulRetCode) {

        //
        // Setting address family to AF_BTH indicates winsock2 to use Bluetooth port
        //
        SockAddrBthLocal.addressFamily = AF_BTH;
        SockAddrBthLocal.port = BT_PORT_ANY;

        //
        // bind() associates a local address and port combination
        // with the socket just created. This is most useful when
        // the application is a server that has a well-known port
        // that clients know about in advance.
        //
        if (SOCKET_ERROR == bind(LocalSocket,
            (struct sockaddr *) &SockAddrBthLocal,
            sizeof(SOCKADDR_BTH))) {
            wprintf(L"=CRITICAL= | bind() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", (ULONG64)LocalSocket, WSAGetLastError());
            ulRetCode = CXN_ERROR;
        }
    }

    if (CXN_SUCCESS == ulRetCode) {

        ulRetCode = getsockname(LocalSocket,
            (struct sockaddr *)&SockAddrBthLocal,
            &iAddrLen);
        if (SOCKET_ERROR == ulRetCode) {
            wprintf(L"=CRITICAL= | getsockname() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", (ULONG64)LocalSocket, WSAGetLastError());
            ulRetCode = CXN_ERROR;
        }
    }

    if (CXN_SUCCESS == ulRetCode) {
        //
        // CSADDR_INFO
        //
        lpCSAddrInfo[0].LocalAddr.iSockaddrLength = sizeof(SOCKADDR_BTH);
        lpCSAddrInfo[0].LocalAddr.lpSockaddr = (LPSOCKADDR)&SockAddrBthLocal;
        lpCSAddrInfo[0].RemoteAddr.iSockaddrLength = sizeof(SOCKADDR_BTH);
        lpCSAddrInfo[0].RemoteAddr.lpSockaddr = (LPSOCKADDR)&SockAddrBthLocal;
        lpCSAddrInfo[0].iSocketType = SOCK_STREAM;
        lpCSAddrInfo[0].iProtocol = BTHPROTO_RFCOMM;

        //
        // If we got an address, go ahead and advertise it.
        //
        ZeroMemory(&wsaQuerySet, sizeof(WSAQUERYSET));
        wsaQuerySet.dwSize = sizeof(WSAQUERYSET);
        wsaQuerySet.lpServiceClassId = (LPGUID)&g_guidServiceClass;

        //
        // Adding a byte to the size to account for the space in the
        // format string in the swprintf call. This will have to change if converted
        // to UNICODE
        //
        res = StringCchLength(szThisComputerName, sizeof(szThisComputerName), &cbInstanceNameSize);
        if (FAILED(res)) {
            wprintf(L"-FATAL- | ComputerName specified is too large\n");
            ulRetCode = CXN_ERROR;
        }
    }

    if (CXN_SUCCESS == ulRetCode) {
        cbInstanceNameSize += sizeof(CXN_INSTANCE_STRING) + 1;
        pszInstanceName = (LPWSTR)HeapAlloc(GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            cbInstanceNameSize);
        if (NULL == pszInstanceName) {
            wprintf(L"-FATAL- | HeapAlloc failed | out of memory | gle = [%d] \n", GetLastError());
            ulRetCode = CXN_ERROR;
        }
    }

    if (CXN_SUCCESS == ulRetCode) {
        StringCbPrintf(pszInstanceName, cbInstanceNameSize, L"%s_APP", szThisComputerName);

        wprintf(L"Instance name '%s'\n", pszInstanceName);

        wsaQuerySet.lpszServiceInstanceName = pszInstanceName;
        wsaQuerySet.lpszComment = L"Example Service instance registered in the directory service through RnR";
        wsaQuerySet.dwNameSpace = NS_BTH;
        wsaQuerySet.dwNumberOfCsAddrs = 1;      // Must be 1.
        wsaQuerySet.lpcsaBuffer = lpCSAddrInfo; // Req'd.

        //
        // As long as we use a blocking accept(), we will have a race
        // between advertising the service and actually being ready to
        // accept connections.  If we use non-blocking accept, advertise
        // the service after accept has been called.
        //
        if (SOCKET_ERROR == WSASetService(&wsaQuerySet, RNRSERVICE_REGISTER, 0)) {
            wprintf(L"=CRITICAL= | WSASetService() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
            ulRetCode = CXN_ERROR;
        }
    }

    //
    // listen() call indicates winsock2 to listen on a given socket for any incoming connection.
    //
    if (CXN_SUCCESS == ulRetCode) {
        if (SOCKET_ERROR == listen(LocalSocket, CXN_DEFAULT_LISTEN_BACKLOG)) {
            wprintf(L"=CRITICAL= | listen() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", (ULONG64)LocalSocket, WSAGetLastError());
            ulRetCode = CXN_ERROR;
        }
    }

    if (CXN_SUCCESS == ulRetCode) {

        for (iCxnCount = 0;
            (CXN_SUCCESS == ulRetCode) && ((iCxnCount < iMaxCxnCycles) || (iMaxCxnCycles == 0));
            iCxnCount++) {

            wprintf(L"\n");

            //
            // accept() call indicates winsock2 to wait for any
            // incoming connection request from a remote socket.
            // If there are already some connection requests on the queue,
            // then accept() extracts the first request and creates a new socket and
            // returns the handle to this newly created socket. This newly created
            // socket represents the actual connection that connects the two sockets.
            //
            ClientSocket = accept(LocalSocket, NULL, NULL);
            if (INVALID_SOCKET == ClientSocket) {
                wprintf(L"=CRITICAL= | accept() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
                ulRetCode = CXN_ERROR;
                break; // Break out of the for loop
            }

            //
            // Read data from the incoming stream
            //
            BOOL bContinue = TRUE;
            pszDataBuffer = (char *)HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY,
                CXN_TRANSFER_DATA_LENGTH);
            if (NULL == pszDataBuffer) {
                wprintf(L"-FATAL- | HeapAlloc failed | out of memory | gle = [%d] \n", GetLastError());
                ulRetCode = CXN_ERROR;
                break;
            }
            pszDataBufferIndex = pszDataBuffer;
            uiTotalLengthReceived = 0;
            while (bContinue && (uiTotalLengthReceived < CXN_TRANSFER_DATA_LENGTH)) {
                //
                // recv() call indicates winsock2 to receive data
                // of an expected length over a given connection.
                // recv() may not be able to get the entire length
                // of data at once.  In such case the return value,
                // which specifies the number of bytes received,
                // can be used to calculate how much more data is
                // pending and accordingly recv() can be called again.
                //
                iLengthReceived = recv(ClientSocket,
                    (char *)pszDataBufferIndex,
                    (CXN_TRANSFER_DATA_LENGTH - uiTotalLengthReceived),
                    0);

                switch (iLengthReceived) {
                case 0: // socket connection has been closed gracefully
                    bContinue = FALSE;
                    break;

                case SOCKET_ERROR:
                    wprintf(L"=CRITICAL= | recv() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
                    bContinue = FALSE;
                    ulRetCode = CXN_ERROR;
                    break;

                default:

                    //
                    // Make sure we have enough room
                    //
                    if (iLengthReceived > (CXN_TRANSFER_DATA_LENGTH - uiTotalLengthReceived)) {
                        wprintf(L"=CRITICAL= | received too much data\n");
                        bContinue = FALSE;
                        ulRetCode = CXN_ERROR;
                        break;
                    }

                    pszDataBufferIndex += iLengthReceived;
                    uiTotalLengthReceived += iLengthReceived;
                    break;
                }
            }

            if (CXN_SUCCESS == ulRetCode) {

                if (CXN_TRANSFER_DATA_LENGTH != uiTotalLengthReceived) {
                    wprintf(L"+WARNING+ | Data transfer aborted mid-stream. Expected Length = [%I64u], Actual Length = [%d]\n", (ULONG64)CXN_TRANSFER_DATA_LENGTH, uiTotalLengthReceived);
                }
                wprintf(L"*INFO* | Received following data string from remote device:\n%s\n", (wchar_t *)pszDataBuffer);

                //
                // Close the connection
                //
                if (SOCKET_ERROR == closesocket(ClientSocket)) {
                    wprintf(L"=CRITICAL= | closesocket() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", (ULONG64)LocalSocket, WSAGetLastError());
                    ulRetCode = CXN_ERROR;
                }
                else {
                    //
                    // Make the connection invalid regardless
                    //
                    ClientSocket = INVALID_SOCKET;
                }
            }
        }
    }

    if (INVALID_SOCKET != ClientSocket) {
        closesocket(ClientSocket);
        ClientSocket = INVALID_SOCKET;
    }

    if (INVALID_SOCKET != LocalSocket) {
        closesocket(LocalSocket);
        LocalSocket = INVALID_SOCKET;
    }

    if (NULL != lpCSAddrInfo) {
        HeapFree(GetProcessHeap(), 0, lpCSAddrInfo);
        lpCSAddrInfo = NULL;
    }
    if (NULL != pszInstanceName) {
        HeapFree(GetProcessHeap(), 0, pszInstanceName);
        pszInstanceName = NULL;
    }

    if (NULL != pszDataBuffer) {
        HeapFree(GetProcessHeap(), 0, pszDataBuffer);
        pszDataBuffer = NULL;
    }

}