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

// #pragma comment(lib, "ws2_32.lib")
// g++ test.cc -lwsock32

static bool isConnect = false;

void Connect();
void Close();
void SendInfo();
void RequestInfo();
void Exit();


int main()
{
    WSADATA wsaData;
    
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cout << "\033[31mWSAStartup failed! Expect WinSock DLL version 2.2!\033[0m" << endl;
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
                Connect();
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
            if (_choice == 1)
            {
                // Request time
                // Send request
                char _request[1024] = "1";
                send(sock, _request, strlen(_request), 0);
                // Receive response
                char _response[1024];
                recv(sock, _response, 1024, 0);
                cout << "Time: " << _response << endl;
            }
            else if (_choice == 2)
            {
                // Request Name
                // Send request
                char _request[1024] = "2";
                send(sock, _request, strlen(_request), 0);
                // Receive response
                char _response[1024];
                recv(sock, _response, 1024, 0);
                cout << "Name: " << _response << endl;
            }
            else if (_choice == 3)
            {
                // Request Clients List
                // Send request
                char _request[1024] = "3";
                send(sock, _request, strlen(_request), 0);
                // Receive response
                char _response[1024];
                recv(sock, _response, 1024, 0);
                cout << "Clients List: " << _response << endl;
            }
            else if (_choice == 4)
            {
                // Send message
                // Send request
                char _request[1024] = "4";
                send(sock, _request, strlen(_request), 0);
                // Receive response
                char _response[1024];
                recv(sock, _response, 1024, 0);
                cout << "Clients List: " << _response << endl;
            }
            else if (_choice == 5)
            {
                // Close connection
                // Send request
                char _request[1024] = "5";
                send(sock, _request, strlen(_request), 0);
                // Receive response
                char _response[1024];
                recv(sock, _response, 1024, 0);
                cout << "Clients List: " << _response << endl;
            }
            else if (_choice == 6)
            {
                // Exit
                break;
            }
            else
            {
                cout << "\033[31m Invalid input!\033[0m" << endl;
            }
        }
    }
}