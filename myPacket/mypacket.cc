#include "mypacket.hh"

uint8_t PerPacket::GetType() { return _type; }

uint8_t PerPacket::GetClientId() { return _client_id; }

std::string PerPacket::GetMessages() { return _message; }

void PerPacket::SetPacket(uint8_t type, uint8_t client_id, std::string message)
{
    _type = type;
    _client_id = client_id;
    _message = message;
}

std::string PerPacket::Package()
{
    std::string packet = "";

    packet.append(1, (_type & 0xf0) | (_client_id & 0x0f));
    packet += _message;
    packet.append(1, ENDSIGNAL);
    return packet;
}

PerPacket decodeRecPacket(const std::string &packet)
{
    PerPacket myPacket;
    if (packet.length() < 2 || static_cast<uint8_t>(packet[packet.length() - 1]) != ENDSIGNAL)
    {
        // // Debug info
        // std::cout << "packet[0]: " << bitset<8>(packet[0]) << std::endl;
        // std::cout << "length: " << packet.length() << std::endl;
        // std::cout << "packet[final]: " << bitset<8>(packet[packet.length() - 1]) << std::endl;
        // std::cout << "Message: " << packet.substr(2, packet.length() - 3) << std::endl;

        std::cout << "\033[31mInvalid packet!\033[0m" << std::endl;
        return PerPacket(0, 0x0f, "Error packet!");
    }
    
    const size_t _len = (packet.length() > MAXLEN) ? MAXLEN : packet.length();
    myPacket.SetPacket(static_cast<uint8_t>(packet[0] & 0xf0), static_cast<uint8_t>(packet[0] & 0x0f), packet.substr(1, _len - 2));

    return myPacket;
}
