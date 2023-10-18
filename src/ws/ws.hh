#include <iostream>
#include <string>
#include <vector>
#include <chrono>

#include "sha1.hh"
#include "b64.hh"

const std::string WS_MAGIC_STRING = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

struct WebSocketFrame {
    bool fin;
    uint8_t opcode;
    bool mask;
    uint64_t payload_length;
    uint32_t masking_key;
    std::vector<uint8_t> payload_data;
};

WebSocketFrame parseWSFrame(const std::vector<uint8_t> &data)
{
    WebSocketFrame frame;
    size_t index = 0;

    // Parse fin bit and opcode
    frame.fin = (data[index] & 0x80) != 0;
    frame.opcode = data[index] & 0x0F;
    index++;

    // Parse mask and payload length
    frame.mask = (data[index] & 0x80) != 0;
    uint8_t length = data[index] & 0x7F;
    index++;

    if (length <= 125) {
        frame.payload_length = length;
    } else if (length == 126) {
        frame.payload_length = (static_cast<uint16_t>(data[index]) << 8) | data[index + 1];
        index += 2;
    } else {
        for (int i=0; i<8; i++) {
            frame.payload_length <<= 8;
            frame.payload_length |= data[index + i];
        }
        index += 8;
    }

    // Parse masking key if mask bit is set
    if (frame.mask) {
        frame.masking_key = (static_cast<uint32_t>(data[index]) << 24) |
                            (static_cast<uint32_t>(data[index + 1]) << 16) |
                            (static_cast<uint32_t>(data[index + 2]) << 8) |
                            static_cast<uint32_t>(data[index + 3]);
        index += 4;
    }

    // Parse payload data
    frame.payload_data.resize(frame.payload_length);
    for (uint64_t i=0; i<frame.payload_length; i++)
        frame.payload_data[i] = data[index + i] ^ ((frame.masking_key >> (8 * (3 - i % 4))) & 0xFF);

    return frame;
}

std::string prepareWSAcceptKey(const std::string &key)
{
    std::string concatenated = trim(key + WS_MAGIC_STRING);
    std::string sh1 = SHA1(concatenated);
    std::string rtn = hexToBase64(sh1);

    //std::cout << "CONCATENATED: " << concatenated << "\nSHA1: " << sh1 << "\n" << "BASE64: " << rtn << "\n" << std::endl;
    return rtn;
}

std::vector<char> frameWSMessage(const std::string &message)
{
    std::vector<char> frame;
    frame.push_back(0x81); // Final fragment, text frame

    if (message.size() <= 125) {
        frame.push_back(static_cast<char>(message.size()));
    } else if (message.size() <= 0xFFFF) {
        frame.push_back(126);
        frame.push_back((message.size() >> 8) & 0xFF);
        frame.push_back(message.size() & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back((message.size() >> (8 * i)) & 0xFF);
        }
    }

    frame.insert(frame.end(), message.begin(), message.end());
    return frame;
}