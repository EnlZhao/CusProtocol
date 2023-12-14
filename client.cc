#include "./myPacket/mypacket.hh"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <bitset>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define LOCK true
#define UNLOCK false

SOCKET _clientSocket;
sockaddr_in _serverAddr; // server information
HANDLE _tid;

bool isConnect = false;
bool _islock = false;
HANDLE _mutex = CreateMutex(NULL, FALSE, NULL);

//! \brief Connect to server
void ConnectServer();

//! \brief Close connection
void CloseConnect(uint8_t type);

//! \brief Send request to server or message to other clients
void SendInfo(uint8_t type);

//! \brief Exit the program
void ExitProg();
//! \brief Receive message
DWORD WINAPI ReceiveMessage(LPVOID lpParameter);
//! \brief Lock or unlock the mutex
void LockOrNot(bool _islock);

int main()
{
    WSADATA wsaData;

    // Initialize WinSock
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cout << "\033[31mWSAStartup failed! Expect WinSock DLL version 2.2!\033[0m" << endl;
        return 0;
    }

    // Affirm WinSock DLL version
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        cout << "\033[31mWSAStartup failed! Expect WinSock DLL version 2.2!\033[0m" << endl;
        WSACleanup();
        return 0;
    }

    while (true)
    {   
        uint16_t _choice;
        // Menu       
        cout << "*********** Menu ***********" << endl;
        if(!isConnect)
        {
            cout << "* 1. Connect to server     *" << endl;
            cout << "* 2. Exit                  *" << endl;
            cout << "****************************" << endl;
            cout << "$ Please input your choice: \n>> ";
            // cin >> _choice;
            fflush(stdin);
            scanf("%d", &_choice);
            if (_choice == 1)
            {
                // Connect to server
                ConnectServer();
            }
            else if (_choice == 2)
            {
                // Exit
                break;
            }
            else
            {
                cout << "\033[31m Invalid input!\033[0m" << endl;
            }
        }
        else
        {
            // pthread_mutex_lock(&_mutex);
            WaitForSingleObject(_mutex, INFINITE);
            cout << "* 1. Request time          *" << endl;
            cout << "* 2. Request Name          *" << endl;
            cout << "* 3. Request Clients List  *" << endl;
            cout << "* 4. Send message          *" << endl;
            cout << "* 5. Close connection      *" << endl;
            cout << "* 6. Exit                  *" << endl;
            cout << "****************************" << endl;
            cout << "Please input your choice: \n";
            //pthread_mutex_unlock(&_mutex);
            ReleaseMutex(_mutex);

            fflush(stdin);
            cout << ">> " ;
            scanf("%d", &_choice);
            
            if (_choice >= 1 && _choice <= 4)
            {
                // Request time | Name | Clients List | Send message
                SendInfo(static_cast<uint8_t>(_choice) + 3 << 4);
            }
            else if (_choice == 5)
            {
                // Close connection
                CloseConnect(CLOSE);
            }
            else if (_choice == 6)
            {
                // Exit
                ExitProg();
                break;
            }
            else
            {
                cout << "\033[31m Invalid input!\033[0m" << endl;
            }
        }
    }
}

void ConnectServer()
{
    if (isConnect)
    {
        cout << "\033[32mYou have already connected to server!\033[0m" << endl;
        return ;
    }
    // Initialize - Apply for socket
    _clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(_clientSocket == INVALID_SOCKET)
    {
        cout << "\033[31mInitialize Socket failed!\033[0m" << endl;
        return ;
    }

    cout << "\nPlease input server IP (Default: 127.0.0.1): \n>> ";
    string server_ip;
    cin >> server_ip;
    // clean buffer
    cin.sync();
    cin.clear();

    cout << "\nPlease input server port (Default: 1638): \n>> ";
    uint16_t server_port;
    cin >> server_port;
    // clean buffer
    cin.sync();
    cin.clear();

    // Initialize - Set server address
    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_port = htons(server_port);
    _serverAddr.sin_addr.S_un.S_addr = inet_addr(server_ip.c_str());

    // // Debug info
    // cout << "Server IP: " << inet_ntoa(_serverAddr.sin_addr) << endl;
    // cout << "Server Port: " << ntohs(_serverAddr.sin_port) << endl;

    // Connect to server
    if (connect(_clientSocket, (sockaddr*)&_serverAddr, sizeof(_serverAddr)) == SOCKET_ERROR)
    {
        cout << "\033[31mConnect to server failed!" << endl;

        // Print Error information
        DWORD dwError=WSAGetLastError();
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            dwError,
            MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0,
            NULL);
        cout << (char*)lpMsgBuf << "\033[0m" << endl;
        LocalFree(lpMsgBuf);

        closesocket(_clientSocket);
        return ;
    }
    else
    {
        cout << "\033[32mConnect to server successfully!\033[0m" << endl;
        isConnect = true;

        // Create sub thread to receive message
        _tid = CreateThread(NULL, 0, ReceiveMessage, &_clientSocket, 0, NULL);

        // Send a package for connecting successfully
        cout << "\033[32mSend a package for connecting successfully!\033[0m" << endl;

        PerPacket send_pack(CONNECT, 0x0f, ""); // connect
        string send_pstr = send_pack.Package();

        // // Debug info
        // cout << "send_pack type : " << bitset<8>(send_pack.GetType()) << endl;
        // cout << "send_pack client_id : " << bitset<8>(send_pack.GetClientId()) << endl;
        // cout << "send_pack message : " << send_pack.GetMessages() << endl;
        // cout << "send_pstr : " << endl;
        // // 打印 string 的二进制形式
        // for (auto i : send_pstr)
        // {
        //     bitset<8> bs(i);
        //     cout << bs << endl;
        // }
        // cout << "send_pstr length: " << send_pstr.length() << endl;

        send(_clientSocket, send_pstr.c_str(), send_pstr.size(), 0);
    }
    return ;
}

void CloseConnect(uint8_t type)
{
    if (!isConnect)
    {
        cout << "\033[32mYou have already closed connection!\033[0m" << endl;
        return;
    }

    // Send a package for closing connection
    PerPacket send_pack = PerPacket(type, 0x0f, ""); // close
    string send_pstr = send_pack.Package();
    send(_clientSocket, send_pstr.c_str(), send_pstr.size(), 0);

    if (type == EXIT)
    {
        closesocket(_clientSocket);
    }
    else
    {
        if (shutdown(_clientSocket, SD_SEND) == SOCKET_ERROR)
        {
            cout << "\033[31mClose connection failed!\033[0m" << endl;
            closesocket(_clientSocket);
            return;
        }
    }
    // Close connection
    isConnect = false;

    // Notifies and waits for the sub threads to close
    CloseHandle(_tid);
    
    cout << "\033[32mClose connection successfully!\033[0m" << endl;
}

void SendInfo(uint8_t type)
{
    int dest_client_id = 0x0f;
    char buf[1022]; // Max Length - 1022 bytes (1024 - 2)
    if (type == SEND_MESSAGE)
    {
        // pthread_mutex_lock(&_mutex);
        WaitForSingleObject(_mutex, INFINITE);

        // Input destination client id
        cout << "\nPlease input destination client id: \n>> ";
        cin >> dest_client_id;

        // // Debug info
        // cout << "Destination client id: " << dest_client_id << endl;

        if (dest_client_id < 0 || dest_client_id > 15)
        {
            cout << "\033[31mInvalid destination client id!\033[0m" << endl;
            // pthread_mutex_unlock(&_mutex);
            ReleaseMutex(_mutex);
            return;
        }

        // Input Message
        // A line feed indicates the end of the inputs
        cout << "\nPlease input message: \n>> ";
        cin.sync();
        cin.clear();
        cin.getline(buf, 1022);

        // pthread_mutex_unlock(&_mutex);
        ReleaseMutex(_mutex);
    }
    
    // Send the message
    PerPacket send_pack = PerPacket(type, static_cast<uint8_t>(dest_client_id), buf);
    string send_pstr = send_pack.Package();
    send(_clientSocket, send_pstr.c_str(), send_pstr.size(), 0);

    // Lock mutex
    LockOrNot(LOCK);

    while(true)
    {
        // wait for reply
        // pthread_mutex_lock(&_mutex);
        WaitForSingleObject(_mutex, INFINITE);
        if (!_islock)
        {
            // pthread_mutex_unlock(&_mutex);
            ReleaseMutex(_mutex);
            break;
        }
        // pthread_mutex_unlock(&_mutex);
        ReleaseMutex(_mutex);
        Sleep(100);
    }
}

void ExitProg()
{
    // Close connection
    CloseConnect(EXIT);
    closesocket(_clientSocket);
    WSACleanup();

    // Exit
    exit(0);
}

DWORD WINAPI ReceiveMessage(LPVOID lpParameter)
{
    SOCKET *sock = (SOCKET*)lpParameter;
    // Store the received message
    char rep_message[1024];
    while (true)
    {
        memset(rep_message, 0, sizeof(rep_message));
        int recv_len = recv(*sock, rep_message, 1024, 0);

        if (recv_len == SOCKET_ERROR || !recv_len) // ERROR
        {
            break;
        }
        PerPacket recv_pack = decodeRecPacket(rep_message);
        auto pack_type = recv_pack.GetType();

        // Lock mutex to print message correctly
        // pthread_mutex_lock(&_mutex);
        WaitForSingleObject(_mutex, INFINITE);
        if (pack_type < REQUEST_TIME || pack_type > SEND_MESSAGE)   // ERROR
        {
            // cout << "\033[34m\n$ Receive: " << recv_pack.GetMessages() << "\033[0m\n" << endl;
            // pthread_mutex_unlock(&_mutex);
            ReleaseMutex(_mutex); 
            continue;
        }
        // Receive message
        if (pack_type == REQUEST_TIME)
        {
            // Request time
            cout << "\033[34m\n$ Current Server Time: \033[0m" << recv_pack.GetMessages() << endl << endl;
        }
        else if (pack_type == REQUEST_SERVER_NAME)
        {
            // Request Name
            cout << "\033[34m\n$ Client Name: \033[0m" << recv_pack.GetMessages() << endl << endl;
        }
        else if (pack_type == REQUEST_CLIENTS_LIST)
        {
            // Request Clients List
            cout << "\033[34m\n$ Current Clients List: \033[0m\n" << recv_pack.GetMessages() << endl << endl;
        }
        else if (pack_type == SEND_MESSAGE)
        {
            // Send message
            cout << "\033[34m\n$ Receive Message: \033[0m\n" << recv_pack.GetMessages() << endl << endl;
            cout << ">> ";
        }
        // pthread_mutex_unlock(&_mutex);
        ReleaseMutex(_mutex); 
        LockOrNot(UNLOCK);   
    }
    return 0;
}

void LockOrNot(bool islock)
{
    // pthread_mutex_lock(&_mutex);
    WaitForSingleObject(_mutex, INFINITE);
    _islock = islock;
    // pthread_mutex_unlock(&_mutex);
    ReleaseMutex(_mutex);
}