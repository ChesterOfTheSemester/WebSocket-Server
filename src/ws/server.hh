/* WebSocket Server 0.1
 * By : Chester Abrahams */

#include <string>
#include <vector>
#include <algorithm>
#include <mutex>
#include <thread>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "utils.hh"
#include "ws.hh"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")

std::vector<SOCKET> clients;
std::mutex          clients_mtx;

class WSServer
{
public:
    WSServer(int PORT)
    {
        _PORT = PORT;
        std::thread thread_server(threadServer, _PORT);
        thread_server.detach();
    }

private:
    int _PORT;

    static bool isConnected(int client)
    {
        char buffer[1];
        int result = recv(client, buffer, sizeof(buffer), MSG_PEEK);

        if (result == 0) {
            std::cout << "Client disconnected" << std::endl;
            return false;
        } else if (result == -1) {
            std::cerr << "Client disconnected due to error: " << errno << std::endl;
            return false;
        }

        return true;
    }

    static void threadClient(SOCKET client)
    {
        char    buffer[1024] = { 0 };
        ssize_t bytesRead = 0;

        while (true)
        {
            if (!isConnected(client)) break;
            if ((bytesRead = recv(client, buffer, sizeof(buffer), 0)) <= 0) continue;

            // Receive data
            WebSocketFrame frame = parseWSFrame(std::vector<uint8_t> (buffer, buffer + 1024));
            std::string    payload (frame.payload_data.begin(), frame.payload_data.end());
            std::cout << "Received data: " << payload << std::endl;

            // Return the data
            std::vector<char> framedMessage = frameWSMessage(payload);
            send(client, &framedMessage[0], framedMessage.size(), 0);

            // You can process the WebSocket data here
            memset(buffer, 0, sizeof(buffer));
        }

        // Remove client socket from array
        clients_mtx.lock();
        auto it = std::find(clients.begin(), clients.end(), client);
        if (it != clients.end()) clients.erase(it);
        clients_mtx.unlock();

        // Close client socket
        closesocket(client);
    }

    static void threadServer(int PORT)
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed." << std::endl;
            return;
        }

        SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in serverAddr = { 0 };
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(PORT);

        bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
        listen(listenSocket, 1);
        std::cout << "Server listening on port " << PORT << "..." << std::endl;

        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        // Wait for incoming connections
        while (true)
        {
            char buffer[1024] = { 0 };

            SOCKET clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
            recv(clientSocket, buffer, 1023, 0);

            std::string request = buffer;
            std::string::size_type key_pos = request.find("Sec-WebSocket-Key: ");

            if (key_pos != std::string::npos)
            {
                // Prepare WS accept key
                std::string key = request.substr(key_pos + 18);
                key = key.substr(0, key.find("\r\n"));
                std::string accept_key = prepareWSAcceptKey(key);

                // Prepare response headers
                std::ostringstream response;
                response << "HTTP/1.1 101 Switching Protocols\n";
                response << "Upgrade: websocket\n";
                response << "Connection: Upgrade\n";
                response << "Sec-WebSocket-Accept: " << accept_key << "\n";
                response << "\r\n";
                send(clientSocket, response.str().c_str(), response.str().length(), 0);

                // Push new client socket to array
                clients_mtx.lock();
                clients.push_back(clientSocket);
                clients_mtx.unlock();

                // Create new thread and run client
                std::thread client(threadClient, clientSocket);
                client.detach();
            }
        }

        closesocket(listenSocket);
        WSACleanup();
    }
};