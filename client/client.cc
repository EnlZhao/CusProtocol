#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <pthread.h>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include "../myPacket/mypacket.hh"

using namespace std;

#define LOCK true
#define UNLOCK false

// #pragma comment(lib, "ws2_32.lib")
// g++ test.cc -lwsock32

SOCKET clientSocket;
sockaddr_in _serverAddr; // server information
pthread_t _tid;


static bool isConnect = false;
static bool _islock = false;
pthread_mutex_t _mutex;

//! \brief Connect to server
bool ConnectServer();

//! \brief Close connection
void CloseConnect();

//! \brief Send request to server or message to other clients
void SendInfo(uint8_t type);

//! \brief Request time
// void RequestInfo();

//! \brief Exit the program
void ExitProg();
//! \brief Receive message
void *ReceiveMessage(void *arg);
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

    // Initialize - Apply for socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock == INVALID_SOCKET)
    {
        cout << "\033[31mInitialize Socket failed!\033[0m" << endl;
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
            cout << "$ Please input your choice: ";
            cin >> _choice;
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
            cout << "* 1. Request time          *" << endl;
            cout << "* 2. Request Name          *" << endl;
            cout << "* 3. Request Clients List  *" << endl;
            cout << "* 4. Send message          *" << endl;
            cout << "* 5. Close connection      *" << endl;
            cout << "* 6. Exit                  *" << endl;
            cout << "****************************" << endl;
            cout << "Please input your choice: ";
            cin >> _choice;
            if (_choice >= 1 && _choice <= 4)
            {
                // Request time | Name | Clients List | Send message
                SendInfo(static_cast<uint8_t>(_choice) + 2 << 4);
            }
            else if (_choice == 5)
            {
                // Close connection
                CloseConnect();
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

bool ConnectServer()
{
    if (isConnect)
    {
        cout << "\033[32mYou have already connected to server!\033[0m" << endl;
        return true;
    }

    cout << "Please input server IP (Default: 127.0.0.1): " << endl;
    string server_ip;
    cin >> server_ip;
    // clean buffer
    cin.clear();

    cout << "Please input server port (Default: 3210): " << endl;
    uint16_t server_port;
    cin >> server_port;
    // clean buffer
    cin.clear();

    // Initialize - Set server address
    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_port = htons(server_port);
    _serverAddr.sin_addr.S_un.S_addr = inet_addr(server_ip.c_str());

    // // print _serverAddr
    // cout << "Server IP: " << inet_ntoa(_serverAddr.sin_addr) << endl;
    // cout << "Server Port: " << ntohs(_serverAddr.sin_port) << endl;

    // Connect to server
    if (connect(clientSocket, (sockaddr*)&_serverAddr, sizeof(_serverAddr)) == SOCKET_ERROR)
    {
        cout << "\033[31mConnect to server failed!\033[0m" << endl;
        closesocket(clientSocket);
        WSACleanup();
        return false;
    }
    else
    {
        cout << "\033[32mConnect to server successfully!\033[0m" << endl;
        isConnect = true;

        // Create sub thread to receive message
        if (pthread_create(&_tid, NULL, ReceiveMessage, &clientSocket) != 0)
        {
            cout << "\033[31mCreate sub thread failed!\033[0m" << endl;
            return false;
        }

        // Send a package for connecting successfully
        MyPacket send_pack(CONNECT); // connect
        string send_pstr = send_pack.Package();
        send(clientSocket, send_pstr.c_str(), send_pstr.size(), 0);
    }
    return true;
}

void CloseConnect()
{
    if (!isConnect)
    {
        cout << "\033[32mYou have already closed connection!\033[0m" << endl;
        return;
    }

    // Send a package for closing connection
    MyPacket send_pack = MyPacket(CLOSE); // close
    string send_pstr = send_pack.Package();
    send(clientSocket, send_pstr.c_str(), send_pstr.size(), 0);

    // Close connection
    closesocket(clientSocket);
    WSACleanup();
    isConnect = false;

    // Notifies and waits for the sub threads to close
    if (pthread_join(_tid, NULL) != 0)
    {
        cout << "\033[31mClose sub thread failed!\033[0m" << endl;
        return;
    }

    cout << "\033[32mClose connection successfully!\033[0m" << endl;

}

void SendInfo(uint8_t type)
{
    uint8_t dest_client_id = 0;
    string buf = ""; // Max Length - 1021 bytes (1024 - 3)
    if (type == SEND_MESSAGE)
    {
        // Input destination client id
        cout << "Please input destination client id: ";
        cin >> dest_client_id;

        cin.clear();

        // Input Message
        // A line feed indicates the end of the inputs
        cin >> buf;
    }
    
    // Send the message
    MyPacket send_pack = MyPacket(type, dest_client_id, buf);
    string send_pstr = send_pack.Package();
    send(clientSocket, send_pstr.c_str(), send_pstr.size(), 0);

    // Lock mutex
    LockOrNot(LOCK);

    while(true)
    {
        // wait for reply
        pthread_mutex_lock(&_mutex);
        if (!_islock)
        {
            pthread_mutex_unlock(&_mutex);
            break;
        }
        pthread_mutex_unlock(&_mutex);
        Sleep(100);
    }
}

void ExitProg()
{
    // Close connection
    CloseConnect();

    // Exit
    exit(0);
}

void *ReceiveMessage(void *arg)
{
    SOCKET *sock = (SOCKET*)arg;
    char rep_message[1024];
    while (true)
    {
        memset(rep_message, 0, sizeof(rep_message));
        int recv_len = recv(*sock, rep_message, 1024, 0);
        if (recv_len == SOCKET_ERROR || !recv_len) // ERROR
        {
            break;
        }

        MyPacket recv_pack = decodeRecPacket(rep_message);

        auto pack_type = recv_pack.GetType();
        // 
        if (pack_type < REQUEST_TIME || pack_type > SEND_MESSAGE)
        {
            // cout << "\033[31mInvalid packet!\033[0m" << endl;
            cout << "\033[31m" << recv_pack.GetMessage() << "\033[0m" << endl;
            continue;
        }

        if (pack_type == REQUEST_TIME)
        {
            // Request time
            cout << "\033[32mCurrent Server Time: \033[0m" << recv_pack.GetMessage() << endl;
        }
        else if (pack_type == REQUEST_SERVER_NAME)
        {
            // Request Name
            cout << "\033[32mClient Name: \033[0m" << recv_pack.GetMessage() << endl;
        }
        else if (pack_type == REQUEST_CLIENTS_LIST)
        {
            // Request Clients List
            cout << "\033[32mCurrent Clients List: \033[0m" << endl;
            cout << recv_pack.GetMessage() << endl;
        }
        else if (pack_type == SEND_MESSAGE)
        {
            // Send message
            cout << "\033[32mMessage: \033[0m" << endl;
            cout << recv_pack.GetMessage() << endl;
        }
        LockOrNot(UNLOCK);   
    }
    return NULL;
}

void LockOrNot(bool islock)
{
    pthread_mutex_lock(&_mutex);
    _islock = islock;
    pthread_mutex_unlock(&_mutex);
}