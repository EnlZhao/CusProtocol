#include "mypacket.hh"

uint8_t MyPacket::GetType() { return _type; }

uint8_t MyPacket::GetClientId() { return _client_id; }

std::string MyPacket::GetMessage() { return _message; }

void MyPacket::SetPacket(uint8_t type, uint8_t client_id, std::string message)
{
    _type = type;
    _client_id = client_id;
    _message = message;
}

std::string MyPacket::Package()
{
    std::string packet = "";

    packet.append(1, (_type & 0xf0) | (_client_id & 0x0f));
    packet.append(1, SEPERATOR);
    packet += _message;
    packet.append(1, ENDSIGNAL);
    return packet;
}

MyPacket decodeRecPacket(const std::string &packet)
{
    MyPacket myPacket;
    if (packet.length() < 3 || packet[1] != SEPERATOR || packet[packet.length() - 1] != ENDSIGNAL)
    {
        std::cout << "\033[31mInvalid packet!\033[0m" << std::endl;
        return MyPacket(0, 0, "Error packet!");
    }
    
    const std::size_t _len = (packet.length() > MAXLEN) ? MAXLEN : packet.length();
    myPacket.SetPacket(static_cast<uint8_t>(packet[0] & 0xf0), static_cast<uint8_t>(packet[0] & 0x0f), packet.substr(2, _len - 2));

    return myPacket;
}