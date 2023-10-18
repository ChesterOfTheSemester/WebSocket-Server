#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

std::string SHA1(const std::string& data)
{
    unsigned int h0 = 0x67452301, h1 = 0xEFCDAB89,
                 h2 = 0x98BADCFE, h3 = 0x10325476,
                 h4 = 0xC3D2E1F0;

    unsigned long long bitLength = 0;
    unsigned int       dataLength = 0;
    unsigned char      dataBlock[64] = {0};

    for (char c : data)
    {
        // Append the character to the data block and update the message length
        dataBlock[dataLength++] = c;
        bitLength += 8;

        // Make sure data block is 64 bytes in size
        if (dataLength == 64)
        {
            unsigned int w[80];

            // Prepare a message schedule of 80 words (32-bit each) from the data block
            for (int i=0; i<16; i++)
                w[i] = (dataBlock[i * 4] << 24) | (dataBlock[i * 4 + 1] << 16) | (dataBlock[i * 4 + 2] << 8) | dataBlock[i * 4 + 3];

            // Extend the message schedule
            for (int i=16; i<80; i++)
                w[i] = ((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]) << 1) | ((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]) >> 31);

            // Initialize five working variables with the current hash values
            unsigned int a = h0, b = h1, c = h2,
                         d = h3, e = h4;

            // Main loop for SHA-1 algorithm
            for (int i=0; i<80; i++)
            {
                unsigned int f, k;

                if (i < 20) {
                    f = (b & c) | ((~b) & d);
                    k = 0x5A827999;
                }
                else if (i < 40) {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                }
                else if (i < 60) {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDC;
                }
                else {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }

                // Update the working variables and the temporary variable
                unsigned int temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
                e = d; d = c;
                c = ((b << 30) | (b >> 2));
                b = a; a = temp;
            }

            // Update the hash values with the results of this block
            h0 += a; h1 += b; h2 += c;
            h3 += d; h4 += e;

            // Reset data block length
            dataLength = 0;
        }
    }

    // Append the bit '1' to the end of the message
    dataBlock[dataLength++] = 0x80;

    // Pad the message to 56 bytes, leaving 8 bytes for the bit length
    if (dataLength > 56)
    {
        while (dataLength < 64) dataBlock[dataLength++] = 0;
        unsigned int w[80];

        for (int i = 0; i < 16; i++)
            w[i] = (dataBlock[i * 4] << 24) | (dataBlock[i * 4 + 1] << 16) | (dataBlock[i * 4 + 2] << 8) | dataBlock[i * 4 + 3];

        for (int i = 16; i < 80; i++)
            w[i] = ((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]) << 1) | ((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]) >> 31);

        unsigned int a = h0, b = h1, c = h2,
                     e = h4, d = h3;

        for (int i=0; i<80; i++)
        {
            unsigned int f;
            unsigned int k;

            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            }
            else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            }
            else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            }
            else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            unsigned int temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
            e = d; d = c;
            c = ((b << 30) | (b >> 2));
            b = a; a = temp;
        }

        h0 += a; h1 += b; h2 += c;
        h3 += d; h4 += e;

        dataLength = 0;
    }

    // Pad the message with '0' bits to 56 bytes
    while (dataLength < 56) dataBlock[dataLength++] = 0;

    // Append the bit length of the original message as a 64-bit integer
    for (int i = 0; i < 8; i++) dataBlock[63 - i] = (bitLength >> (i * 8)) & 0xFF;

    // Prepare the final hash value as a hexadecimal string
    unsigned int w[80];
    for (int i=0; i<16; i++) w[i] = (dataBlock[i * 4] << 24) | (dataBlock[i * 4 + 1] << 16) | (dataBlock[i * 4 + 2] << 8) | dataBlock[i * 4 + 3];
    for (int i=16; i<80; i++) w[i] = ((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]) << 1) | ((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]) >> 31);

    unsigned int a = h0, b = h1,
                 c = h2, d = h3, e = h4;

    // Main  loop for the last block
    for (int i=0; i<80; i++)
    {
        unsigned int f, k;

        // Determine the function (f) and constant (k) for this iteration.
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        }
        else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        }
        else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        }
        else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }

        // Update the working variables and the temporary variable
        unsigned int temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
        e = d; d = c;
        c = ((b << 30) | (b >> 2));
        b = a; a = temp;
    }

    // Update the hash values with the results of the last block
    h0 += a; h1 += b; h2 += c;
    h3 += d; h4 += e;

    // Prepare the final SHA-1 hash value as a hexadecimal string
    std::ostringstream oss;
    oss << std::hex << std::setw(8) << std::setfill('0');
    oss << h0 << h1 << h2 << h3 << h4;

    return oss.str();
}