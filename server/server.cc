#include "../myPacket/mypacket.hh"
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <ctime>
#include <map>
#include <unistd.h>
#include <winsock2.h>
#include <ws2tcpip.h>
// #include <bitset>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define SERVERPORT 1638 
#define MAXLISTEN 12

typedef uint16_t id; // 0 - 0x0f (0x0f represents invalid!)
typedef string ip_addr;
typedef uint16_t port;
typedef bool occupied;
id _idQueue = 0; // 0 - 0x0e

SOCKET _sockfd;
HANDLE _mutex = CreateMutex(NULL, FALSE, NULL);

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

DWORD WINAPI SubThread(LPVOID lpParameter);

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
        cout << "\033[31mFail to create a socket." << endl;
        // Print error information
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

        return -1;
    }

    // Input Port
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
    cout << "\033[32m$ Bind Port [1638] successfully!\033[0m" << endl;

    // Listen
    if (listen(_sockfd, 20) == -1)
    {
        cout << "\033[31mFail to listen.\033[0m" << endl;
        return -1;
    }
    cout << "\033[32m$ Listening!\033[0m" << endl;

    cout << "\033[32mServer is running...\033[0m" << endl;
    while (true)
    {
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        // Accept connection
        SOCKET clientSockfd = accept(_sockfd, (sockaddr *)&clientAddr, (socklen_t *)&clientAddrLen);
        
        // lock mutex to add to client list correctly 
        // pthread_mutex_lock(&_mutex);
        WaitForSingleObject(_mutex, INFINITE);

        // Add to client list
        id clientID = _idQueue;
        _idQueue = (++_idQueue) % 15;
        // // Debug info
        // cout << "clientID: " << clientID << endl;
        // cout << "_clientOccupied[clientID]: " << _clientOccupied[clientID] << endl;

        // Search for a valid clientID
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
        // // Debug info
        // cout << "clientID: " << clientID << endl;
        // cout << "_clientOccupied[clientID]: " << _clientOccupied[clientID] << endl;
        
        // Add to client list
        ClientInfo clientInfo = ClientInfo(clientID, clientSockfd, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        _clientList.insert(pair<id, ClientInfo>(clientID, clientInfo));

        cout << "\033[32mClient \033[0m" << clientID << "\033[32m : \033[0m" << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << "\033[32m connects successfully!\033[0m" << endl;
        // pthread_mutex_unlock(&_mutex);
        ReleaseMutex(_mutex);(&_mutex);

        // Create sub thread to receive message
        CreateThread(NULL, 0, SubThread, (LPVOID)&clientInfo, 0, NULL);
    }
    closesocket(_sockfd);
    WSACleanup();
    return 0;
}

DWORD WINAPI SubThread(LPVOID lpParameter)
{
    ClientInfo clientInfo = *(ClientInfo *)lpParameter;

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
        string rep_message_str = rep_message;
        PerPacket recv_pack = decodeRecPacket(rep_message_str);

        auto pack_type = recv_pack.GetType();
        PerPacket reply_pack;

        // // Debug info
        // cout << "\n\033[32mReceive: \033[0m" << endl;
        // cout << "bitset<8>Type: " << bitset<8>(pack_type) << endl;
        // cout << "Hex Type: " << hex << static_cast<int>(pack_type) << endl;

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
                // // Debug info
                // clients_list += "Client Occupied: " + to_string(_clientOccupied[it->first]) + " | ";
                clients_list += "Client ID: " + to_string(it->first) + " --> " + it->second._clientIP + ":" + to_string(it->second._clientPort) + "\n";
            }
            reply_pack.SetPacket(REQUEST_CLIENTS_LIST, 0x0f, clients_list);
        }
        else if (pack_type == SEND_MESSAGE)
        {
            id dest_id = static_cast<id>(recv_pack.GetClientId());

            // // Debug info
            // cout << "dest_id: " << dest_id << endl;
            // cout << "clientOccupied: " << _clientOccupied[dest_id] << endl;

            if (_clientOccupied[dest_id])
            {
                // Send message
                auto it = _clientList.find(dest_id);
                PerPacket message_pack(SEND_MESSAGE, 0x0f, recv_pack.GetMessages());
                string message = message_pack.Package();
                send(it->second._clientSockfd, message.c_str(), message.size(), 0);
                reply_pack.SetPacket(SEND_MESSAGE, 0x0f, "Send message successfully!");
                cout << "\033[32mForward the message to Client [" << dest_id <<  "]\033[0m" << endl;
            }
            else // Send back to source client
            {
                reply_pack.SetPacket(SEND_MESSAGE, 0x0f, "Invalid target client id!");
            }
        }
        else if (pack_type == CLOSE)
        {
            cout << "\033[32mClient \033[0m" << clientID << "\033[32m: \033[0m" << clientIP << ":" << clientPort << "\033[32m disconnects successfully!\033[0m" << endl;
            // break;
        }
        else if (pack_type == CONNECT)
        {
            reply_pack.SetPacket(CONNECT, 0x0f, "Connected!");
        }
        else if(pack_type == EXIT)
        {
            cout << "\033[32mClient \033[0m" << clientID << "\033[32m: \033[0m" << clientIP << ":" << clientPort << "\033[32m exits successfully!\033[0m" << endl;
        }
        else
        {
            reply_pack.SetPacket(0, 0x0f, "Invalid packet!");
        }
        // // Debug info
        // cout << "\n\033[32mSend: \033[0m\n>>\n" << reply.substr(1, reply.length() - 2) << endl;
        if (pack_type == CLOSE || pack_type == EXIT)
        {
            break;
        }
        string reply = reply_pack.Package();
        send(clientSockfd, reply.c_str(), reply.size(), 0);
    }
    closesocket(clientSockfd);
    _clientOccupied[clientID] = false;
    _clientList.erase(clientID);
    _idQueue = (!_idQueue) ? 14 : _idQueue - 1;
    return 0;
}