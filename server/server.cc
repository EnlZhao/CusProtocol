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
#include "../myPacket/mypacket.hh"

using namespace std;

#define ServerPort 1638 

pthread_mutex_t mutex;
