#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <pthread.h>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <bitset>
// #include "../myPacket/mypacket.hh"
#pragma comment(lib, "ws2_32.lib")

using namespace std;

///////////////////////////////
#include <string>
#define ENDSIGNAL 0b11111111
#define MAXLEN 1024
#define CONNECT 0x10
#define CLOSE 0x20
#define EXIT 0x30
#define REQUEST_TIME 0x40
#define REQUEST_SERVER_NAME 0x50
#define REQUEST_CLIENTS_LIST 0x60
#define SEND_MESSAGE 0x70

class MyPacket
{
private:

    //! \brief Represent the type of packet
    //! \details  Only use high 4 bits
    //! \details  8th bit is 0 -> Request and Indicate packet
    //! \details  8th bit is 1 -> Response packet
    /*
    +---0x00 Error
    +---0x10 Connect
    +---0x20 Close
    +---0x30 Exit
    +---0x40 Request Time
    +---0x50 Request Server Name
    +---0x60 Request Clients List
    +---0x70 Send Message
    */
    uint8_t _type;

    //! \brief Represent the client id
    //! \details only use low 4 bits - Max 15 clients (Default all 1 -> 0x0F)
    uint8_t _client_id;

    string _message;
public:
    MyPacket() : _type(0), _client_id(0x0f), _message("") {}
    MyPacket(uint8_t type, uint8_t client_id = 0x0f, string message= "") : _type(type), _client_id(client_id), _message(message) { }
    uint8_t GetType();
    uint8_t GetClientId();
    string GetMessage();
    void SetPacket(uint8_t type, uint8_t client_id = 0x0f, string message = "");
    string Package();
};

uint8_t MyPacket::GetType() { return _type; }

uint8_t MyPacket::GetClientId() { return _client_id; }

string MyPacket::GetMessage() { return _message; }

void MyPacket::SetPacket(uint8_t type, uint8_t client_id, string message)
{
    _type = type;
    _client_id = client_id;
    _message = message;
}

string MyPacket::Package()
{
    string packet = "";

    packet.append(1, (_type & 0xf0) | (_client_id & 0x0f));
    packet += _message;
    packet.append(1, ENDSIGNAL);
    return packet;
}

MyPacket decodeRecPacket(const string &packet)
{
    MyPacket myPacket;
    if (packet.length() < 2 || static_cast<uint8_t>(packet[packet.length() - 1]) != ENDSIGNAL)
    {
        // // Debug info
        // cout << "packet[0]: " << bitset<8>(packet[0]) << endl;
        // cout << "length: " << packet.length() << endl;
        // cout << "packet[final]: " << bitset<8>(packet[packet.length() - 1]) << endl;
        // cout << "Message: " << packet.substr(2, packet.length() - 3) << endl;

        cout << "\033[31mInvalid packet!\033[0m" << endl;
        return MyPacket(0, 0x0f, "Error packet!");
    }
    
    const size_t _len = (packet.length() > MAXLEN) ? MAXLEN : packet.length();
    myPacket.SetPacket(static_cast<uint8_t>(packet[0] & 0xf0), static_cast<uint8_t>(packet[0] & 0x0f), packet.substr(1, _len - 2));

    return myPacket;
}

/////////////////////////////

#define LOCK true
#define UNLOCK false

SOCKET _clientSocket;
sockaddr_in _serverAddr; // server information
pthread_t _tid;

bool isConnect = false;
bool _islock = false;
pthread_mutex_t _mutex;

//! \brief Connect to server
void ConnectServer();

//! \brief Close connection
void CloseConnect(uint8_t type);

//! \brief Send request to server or message to other clients
void SendInfo(uint8_t type);

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
            pthread_mutex_lock(&_mutex);
            cout << "* 1. Request time          *" << endl;
            cout << "* 2. Request Name          *" << endl;
            cout << "* 3. Request Clients List  *" << endl;
            cout << "* 4. Send message          *" << endl;
            cout << "* 5. Close connection      *" << endl;
            cout << "* 6. Exit                  *" << endl;
            cout << "****************************" << endl;
            cout << "Please input your choice: \n";
            pthread_mutex_unlock(&_mutex);

            // cin >> _choice;
            // pthread_mutex_lock(&_mutex);
            fflush(stdin);
            cout << ">> " ;
            scanf("%d", &_choice);
            // pthread_mutex_unlock(&_mutex);
            
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

    cout << "\nPlease input server port (Default: 3210): \n>> ";
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
        if (pthread_create(&_tid, NULL, ReceiveMessage, &_clientSocket) != 0)
        {
            cout << "\033[31mCreate sub thread failed!\033[0m" << endl;
            return ;
        }

        // Send a package for connecting successfully
        cout << "\033[32mSend a package for connecting successfully!\033[0m" << endl;

        MyPacket send_pack(CONNECT, 0x0f, ""); // connect
        string send_pstr = send_pack.Package();

        // // Debug info
        // cout << "send_pack type : " << bitset<8>(send_pack.GetType()) << endl;
        // cout << "send_pack client_id : " << bitset<8>(send_pack.GetClientId()) << endl;
        // cout << "send_pack message : " << send_pack.GetMessage() << endl;
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
    MyPacket send_pack = MyPacket(type, 0x0f, ""); // close
    string send_pstr = send_pack.Package();
    send(_clientSocket, send_pstr.c_str(), send_pstr.size(), 0);

    closesocket(_clientSocket);
    // Close connection
    isConnect = false;

    // Notifies and waits for the sub threads to close
    pthread_join(_tid, NULL);
    
    cout << "\033[32mClose connection successfully!\033[0m" << endl;
}

void SendInfo(uint8_t type)
{
    int dest_client_id = 0x0f;
    char buf[1022]; // Max Length - 1022 bytes (1024 - 2)
    if (type == SEND_MESSAGE)
    {
        pthread_mutex_lock(&_mutex);

        // Input destination client id
        cout << "\nPlease input destination client id: \n>> ";
        cin >> dest_client_id;

        // // Debug info
        // cout << "Destination client id: " << dest_client_id << endl;

        if (dest_client_id < 0 || dest_client_id > 15)
        {
            cout << "\033[31mInvalid destination client id!\033[0m" << endl;
            pthread_mutex_unlock(&_mutex);
            return;
        }

        // Input Message
        // A line feed indicates the end of the inputs
        cout << "\nPlease input message: \n>> ";
        cin.sync();
        cin.clear();
        cin.getline(buf, 1022);

        pthread_mutex_unlock(&_mutex);
    }
    
    // Send the message
    MyPacket send_pack = MyPacket(type, static_cast<uint8_t>(dest_client_id), buf);
    string send_pstr = send_pack.Package();
    send(_clientSocket, send_pstr.c_str(), send_pstr.size(), 0);

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
    CloseConnect(EXIT);
    closesocket(_clientSocket);
    WSACleanup();

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

        pthread_mutex_lock(&_mutex);
        if (pack_type < REQUEST_TIME || pack_type > SEND_MESSAGE)
        {
            // cout << "\033[34m\n$ Receive: " << recv_pack.GetMessage() << "\033[0m\n" << endl;
            pthread_mutex_unlock(&_mutex); 
            continue;
        }

        if (pack_type == REQUEST_TIME)
        {
            // Request time
            cout << "\033[34m\n$ Current Server Time: \033[0m" << recv_pack.GetMessage() << endl << endl;
        }
        else if (pack_type == REQUEST_SERVER_NAME)
        {
            // Request Name
            cout << "\033[34m\n$ Client Name: \033[0m" << recv_pack.GetMessage() << endl << endl;
        }
        else if (pack_type == REQUEST_CLIENTS_LIST)
        {
            // Request Clients List
            cout << "\033[34m\n$ Current Clients List: \033[0m\n" << recv_pack.GetMessage() << endl << endl;
        }
        else if (pack_type == SEND_MESSAGE)
        {
            // Send message
            cout << "\033[34m\n$ Receive Message: \033[0m\n" << recv_pack.GetMessage() << endl << endl;
            cout << ">> ";
        }
        pthread_mutex_unlock(&_mutex); 
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