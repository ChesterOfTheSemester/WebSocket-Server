#include <iostream>
#include <string>

const std::string base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string hexToBase64(const std::string& hexStr)
{
    std::string base64Str;
    int val = 0;
    int bits = 0;

    for (char c : hexStr) {
        if (c >= '0' && c <= '9') val = val * 16 + (c - '0');
        else if (c >= 'A' && c <= 'F') val = val * 16 + (c - 'A' + 10);
        else if (c >= 'a' && c <= 'f') val = val * 16 + (c - 'a' + 10);
        else return "";

        bits += 4;

        while (bits >= 6) {
            bits -= 6;
            base64Str.push_back(base64Chars[(val >> bits) & 0x3F]);
        }
    }

    if (bits > 0) {
        val <<= 6 - bits;
        base64Str.push_back(base64Chars[val & 0x3F]);
        while (bits < 6) base64Str.push_back('='), bits += 2;
    }

    return base64Str;
}
