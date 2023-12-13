#!/bin/bash

rm *.exe
# build
g++ server.cc -o server.exe -lpthread -lwsock32
g++ client.cc -o client.exe -lpthread -lwsock32
