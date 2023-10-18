#include <iostream>
#include <chrono>

#include "ws/server.hh"

int main()
{
    // Start connection threads
    WSServer server(4732);

    // Run code
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "Total connections: " << server.clients->size() << std::endl;
    }


    return 0;
}
