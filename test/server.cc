#include <iostream>
#include <cstring>
#include <vector>
#include <mutex>
#include <ctime>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <winsock2.h>
#include <ws2tcpip.h>
using namespace std;

#define SERVERPORT 1638 
#define MAXLISTEN 12

///////////////////////////////
#include <string>
#define SEPERATOR 0b11111111
#define ENDSIGNAL 0b00000000
#define MAXLEN 1024
#define CONNECT 0x10
#define CLOSE 0x20
#define REQUEST_TIME 0x30
#define REQUEST_SERVER_NAME 0x40
#define REQUEST_CLIENTS_LIST 0x50
#define SEND_MESSAGE 0x60

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
    +---0x30 Request Time
    +---0x40 Request Server Name
    +---0x50 Request Clients List
    +---0x60 Send Message
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

//! \brief Decode received packet to MyPacket
MyPacket decodeRecPacket(const string& packet);

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
    packet.append(1, SEPERATOR);
    packet += _message;
    packet.append(1, ENDSIGNAL);
    return packet;
}

MyPacket decodeRecPacket(const string &packet)
{
    MyPacket myPacket;
    if (packet.length() < 3 || packet[1] != SEPERATOR || packet[packet.length() - 1] != ENDSIGNAL)
    {
        cout << "\033[31mInvalid packet!\033[0m" << endl;
        return MyPacket(0, 0, "Error packet!");
    }
    
    const size_t _len = (packet.length() > MAXLEN) ? MAXLEN : packet.length();
    myPacket.SetPacket(static_cast<uint8_t>(packet[0] & 0xf0), static_cast<uint8_t>(packet[0] & 0x0f), packet.substr(2, _len - 2));

    return myPacket;
}


/////////////////////////////

typedef uint8_t id; // 0 - 0x0f
typedef string ip_addr;
typedef uint16_t port;
typedef bool occupied;
id _idQueue = 0;

SOCKET _sockfd;
pthread_mutex_t _mutex;

struct ClientInfo
{
    // Client information
    id _clientID;
    SOCKET _clientSockfd;
    ip_addr _clientIP;
    port _clientPort;
    // Constructor
    ClientInfo(id ID, SOCKET Sockfd, ip_addr IP, port Port) : _clientID(ID), _clientSockfd(Sockfd), _clientIP(IP), _clientPort(Port) {}
};
// Client list
map<id, ClientInfo> _clientList;
occupied _clientOccupied[15] = {false};

void *SubThread(void *arg);

int main()
{
    WSADATA wsaData;

    // Initialize WinSock
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cout << "\033[31mWSAStartup failed! Expect WinSock DLL version 2.2!\033[0m" << endl;
        return 0;
    }

    // Initialize
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sockfd == -1)
    {
        cout << "\033[31mFail to create a socket.\033[0m" << endl;
        // 打印错误信息
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
        cout << (char*)lpMsgBuf << endl;

        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVERPORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind
    if (bind(_sockfd, (sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        cout << "\033[31mFail to bind.\033[0m" << endl;
        return -1;
    }

    // Listen
    if (listen(_sockfd, 20) == -1)
    {
        cout << "\033[31mFail to listen.\033[0m" << endl;
        return -1;
    }

    cout << "\033[32mServer is running...\033[0m" << endl;
    while (true)
    {
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SOCKET clientSockfd = accept(_sockfd, (sockaddr *)&clientAddr, (socklen_t *)&clientAddrLen);
        
        pthread_mutex_lock(&_mutex);
        // Add to client list
        id clientID = _idQueue;
        _idQueue = (++_idQueue) % 15;
        if (_clientOccupied[clientID])
        {
            while (_clientOccupied[_idQueue] && clientID != _idQueue)
            {
                _idQueue = (++_idQueue) % 15;
            }
            clientID = _idQueue;
        }
        if (_clientOccupied[clientID])
        {
            cout << "\033[31mClient list is full!\033[0m" << endl;
            continue;
        }
        _clientOccupied[clientID] = true;
        
        ClientInfo clientInfo = ClientInfo(clientID, clientSockfd, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        _clientList.insert(pair<id, ClientInfo>(clientID, clientInfo));

        cout << "\033[32mClient \033[0m" << clientID <<  "\033[32m : \033[0m" << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << "\033[32m connect successfully!\033[0m" << endl;
        pthread_mutex_unlock(&_mutex);

        // Create sub thread to receive message
        pthread_t tid;
        pthread_create(&tid, NULL, SubThread, &clientInfo);
    }
}

void *SubThread(void *arg)
{
    ClientInfo clientInfo = *(ClientInfo *)arg;

    SOCKET clientSockfd = clientInfo._clientSockfd;
    ip_addr clientIP = clientInfo._clientIP;
    port clientPort = clientInfo._clientPort;
    id clientID = clientInfo._clientID;

    char rep_message[1024];

    while (true)
    {
        memset(rep_message, 0, 1024);
        int recv_len = recv(clientSockfd, rep_message, 1024, 0);
        if (recv_len == SOCKET_ERROR || !recv_len)
        {
            break;
        }
        MyPacket recv_pack = decodeRecPacket(rep_message);

        auto pack_type = recv_pack.GetType();
        MyPacket reply_pack;
        if (pack_type == REQUEST_TIME)
        {
            time_t rawtime;
            struct tm *info;
            char buffer[80];
            time(&rawtime);
            info = localtime(&rawtime);
            strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", info);
            reply_pack.SetPacket(REQUEST_TIME, 0x0f, buffer);
        }
        else if (pack_type == REQUEST_SERVER_NAME)
        {
            char server_name[80];
            gethostname(server_name, 80);
            reply_pack.SetPacket(REQUEST_SERVER_NAME, 0x0f, server_name);
        }
        else if (pack_type == REQUEST_CLIENTS_LIST)
        {
            // ID, IP, Port
            string clients_list = "";
            for (auto it = _clientList.begin(); it != _clientList.end(); it++)
            {
                clients_list += to_string(it->first) + " : " + it->second._clientIP + ":" + to_string(it->second._clientPort) + "\n";
            }
            reply_pack.SetPacket(REQUEST_CLIENTS_LIST, 0x0f, clients_list);
        }
        else if (pack_type == SEND_MESSAGE)
        {
            id dest_id = recv_pack.GetClientId();
            if (_clientOccupied[dest_id])
            {
                // Send message
                auto it = _clientList.find(dest_id);
                MyPacket message_pack(SEND_MESSAGE, 0x0f, recv_pack.GetMessage());
                string message = message_pack.Package();
                send(it->second._clientSockfd, message.c_str(), message.size(), 0);
                reply_pack.SetPacket(SEND_MESSAGE, 0x0f, "Send message successfully!");
            }
            else // Send back to source client
            {
                reply_pack.SetPacket(SEND_MESSAGE, 0x0f, "Invalid target client id!");
            }
        }
        else if (pack_type == CLOSE)
        {
            reply_pack.SetPacket(CLOSE, 0x0f, "Close connection");
            break;
        }
        else if (pack_type == CONNECT)
        {
            reply_pack.SetPacket(CONNECT, 0x0f, "Connect");
        }
        else
        {
            reply_pack.SetPacket(0, 0x0f, "Invalid packet!");
        }

        string reply = reply_pack.Package();

        cout << "\033[32m Reply: \033[0m\n" << reply << endl;

        send(clientSockfd, reply.c_str(), reply.size(), 0);

        if (reply_pack.GetType() == CLOSE)
        {
            cout << "\033[32mClient \033[0m" << clientID << "\033[32m : \033[0m" << clientIP << ":" << clientPort << "\033[32m disconnect successfully!\033[0m" << endl;
            break;
        }
        close(clientSockfd);
        _clientOccupied[clientID] = false;
    }
    return NULL;
}