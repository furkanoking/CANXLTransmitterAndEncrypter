#include "CANXL.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char** argv) {
    const std::string interface_name = (argc > 1) ? argv[1] : "Aleyna";
    const int tx_id = (argc > 2) ? std::stoi(argv[2], nullptr, 0) : 0x13;

    CANXL canxl;
    const std::string socket_name = "CANXL_TX_SOCKET";

    if (!canxl.CreateSocket(socket_name)) {
        std::cerr << "CreateSocket failed" << std::endl;
        return 1;
    }

    if (!canxl.setNetworkInterfaceUp(interface_name, socket_name)) {
        std::cerr << "setNetworkInterfaceUp failed for interface: " << interface_name << std::endl;
        return 1;
    }

    const std::string message = "Hello from CANXL sender";
    std::cout << "Sending to ID=0x" << std::hex << tx_id << std::dec
              << " payload=\"" << message << "\" len=" << message.size() << std::endl;

    canxl.SendMessage(socket_name, tx_id, static_cast<int>(message.size()), message.data());

    // Allow background sender thread to flush before process exits.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return 0;
}
