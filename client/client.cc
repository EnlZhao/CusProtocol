#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <pthread.h>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include "../myPacket/mypacket.hh"

#pragma comment(lib, "ws2_32.lib")
static bool isConnect = false;

void Connect();


int main()
{
    WSADATA wsaData;
    
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cout << "WSAStartup failed! Expect WinSock DLL version 2.2!\n" << std::endl;
        return 0;
    }

    // Initialize - Apply for socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock == INVALID_SOCKET)
    {
        std::cout << "Initialize Socket failed!" << std::endl;
        return 0;
    }

    while (true)
    {   
        uint16_t _choice;
        // Menu       
        std::cout << "*********** Menu ***********" << std::endl;
        if(!isConnect)
        {
            std::cout << "* 1. Connect to server     *" << std::endl;
            std::cout << "* 2. Exit                  *" << std::endl;
            std::cout << "****************************" << std::endl;
            std::cout << "$ Please input your choice: ";
            std::cin >> _choice;
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
                std::cout << " Invalid input!" << std::endl;
            }
        }
        else
        {
            std::cout << "* 1. Request time          *" << std::endl;
            std::cout << "* 2. Request Name          *" << std::endl;
            std::cout << "* 3. Request Clients List  *" << std::endl;
            std::cout << "* 4. Send message          *" << std::endl;
            std::cout << "* 5. Close connection      *" << std::endl;
            std::cout << "* 6. Exit                  *" << std::endl;
            std::cout << "****************************" << std::endl;
            std::cout << "Please input your choice: ";
            std::cin >> _choice;
            if (_choice == 1)
            {
                // Request time
                // Send request
                char _request[1024] = "1";
                send(sock, _request, strlen(_request), 0);
                // Receive response
                char _response[1024];
                recv(sock, _response, 1024, 0);
                std::cout << "Time: " << _response << std::endl;
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
                std::cout << "Name: " << _response << std::endl;
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
                std::cout << "Clients List: " << _response << std::endl;
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
                std::cout << "Clients List: " << _response << std::endl;
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
                std::cout << "Clients List: " << _response << std::endl;
            }
            else if (_choice == 6)
            {
                // Exit
                break;
            }
            else
            {
                std::cout << " Invalid input!" << std::endl;
            }
        }
    }
}