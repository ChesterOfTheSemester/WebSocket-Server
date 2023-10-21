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

#include "utils/utils.hh"
#include "ws.hh"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")

std::vector<SOCKET> ws_clients; std::mutex ws_clients_mtx;
std::vector<std::pair<SOCKET, std::vector<char>>> ws_input_buffer; std::mutex ws_input_buffer_mtx;
std::vector<std::pair<SOCKET, std::string>> ws_output_buffer; std::mutex ws_output_buffer_mtx;

class WSServer
{
public:
    std::vector<SOCKET> *clients = &ws_clients;

    WSServer(int PORT)
    {
        // Create thread for listening server
        _PORT = PORT;
        std::thread thread_server(threadServer, _PORT);
        thread_server.detach();

        // Create thread for handling all connections and buffers
        std::thread l(mainLoop);
        l.detach();
    }

    static void mainLoop()
    {
        while (true)
        {
            // Iterate input buffer vector
            for (const auto &clients : ws_input_buffer)
                try {
                    // Print message
                    if (!&clients) continue;
                    SOCKET client =   clients.first;
                    std::vector<char> data = clients.second;
                    std::string       data_str(data.begin(), data.end());
                    std::cout << "Received from client: " << data_str << std::endl;

                    // Return the message
                    emit(client, data_str);

                    // Erase message
                    ws_input_buffer_mtx.lock();
                    for (auto i = ws_input_buffer.begin(); i != ws_input_buffer.end(); ++i)
                        if (i->second == data) {
                            ws_input_buffer.erase(i);
                            break;
                        }
                    ws_input_buffer_mtx.unlock();
                } catch (const std::exception& e) {}

            // Iterate output buffer vector and send message data to respective clients
            for (const auto &clients : ws_output_buffer)
                try {
                    SOCKET client =     clients.first;
                    std::vector<char>   framedMessage = frameWSMessage(clients.second);

                    send(client, &framedMessage[0], framedMessage.size(), 0);

                    // Erase message
                    ws_output_buffer_mtx.lock();
                    for (auto i = ws_output_buffer.begin(); i != ws_output_buffer.end(); ++i)
                        if (i->second == clients.second) {
                            ws_output_buffer.erase(i);
                            break;
                        }
                    ws_output_buffer_mtx.unlock();
                } catch (const std::exception& e) {}
        }
    }

    static void emit(SOCKET client, std::string data)
    {
        ws_output_buffer_mtx.lock();
        ws_output_buffer.push_back(std::make_pair(client, data));
        ws_output_buffer_mtx.unlock();
    }

    static void disconnect(SOCKET client)
    {
        _disconnect(client);
    }

private:
    int _PORT;

    static void _disconnect(SOCKET client)
    {
        closesocket(client);
    }

    static bool isConnected(int client)
    {
        char buffer[1];
        int result = recv(client, buffer, sizeof(buffer), MSG_PEEK);

        switch (result) {
            case 0:  std::cout << "Client disconnected" << std::endl; return false;
            case -1: std::cerr << "Client disconnected due to error: " << errno << std::endl; return false;
            default: return true;
        }
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

            // Push client and payload to input buffer
            ws_input_buffer_mtx.lock();
            ws_input_buffer.push_back(std::make_pair(client, std::vector<char>(payload.begin(), payload.begin() + payload.length())));
            ws_input_buffer_mtx.unlock();

            memset(buffer, 0, sizeof(buffer));
        }

        // Remove client socket from vector
        ws_clients_mtx.lock();
        auto it = std::find(ws_clients.begin(), ws_clients.end(), client);
        if (it != ws_clients.end()) ws_clients.erase(it);
        ws_clients_mtx.unlock();

        // Close client socket
        closesocket(client);
    }

    static void threadServer(int PORT)
    {
        // Todo: add error handling
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData) != 0;
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

                // Push new client socket to vector
                ws_clients_mtx.lock();
                ws_clients.push_back(clientSocket);
                ws_clients_mtx.unlock();

                // Create new thread and run client
                std::thread client(threadClient, clientSocket);
                client.detach();
            }
        }

        closesocket(listenSocket);
        WSACleanup();
    }
};