#pragma once
#include <iostream>
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

    std::string _message;
public:
    MyPacket() : _type(0), _client_id(0x0f), _message("") {}
    MyPacket(uint8_t type, uint8_t client_id = 0x0f, std::string message= "") : _type(type), _client_id(client_id), _message(message) { }
    uint8_t GetType();
    uint8_t GetClientId();
    std::string GetMessage();
    void SetPacket(uint8_t type, uint8_t client_id = 0x0f, std::string message = "");
    std::string Package();
};

//! \brief Decode received packet to MyPacket
MyPacket decodeRecPacket(const std::string& packet);