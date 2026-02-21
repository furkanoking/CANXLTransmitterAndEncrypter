#include "CANXL.h"

#include <chrono>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

static void on_receive(CANXLStruct frame) {
    std::cout << "[RX] id=0x" << std::hex << frame.CANID << std::dec
              << " len=" << frame.LENGTH << " data(hex)=";

    for (int i = 0; i < frame.LENGTH; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(frame.DATA[i]) << ' ';
    }
    std::cout << std::dec << " | data(ascii)=\"";

    for (int i = 0; i < frame.LENGTH; ++i) {
        const unsigned char c = frame.DATA[i];
        std::cout << (std::isprint(c) ? static_cast<char>(c) : '.');
    }
    std::cout << "\"" << std::endl;
    std::cout << "There is a message arrived. \n";
}

int main(int argc, char** argv) {
    const std::string interface_name = (argc > 1) ? argv[1] : "Aleyna";
    const int rx_id = (argc > 2) ? std::stoi(argv[2], nullptr, 0) : 0x13;

    CANXL canxl;
    const std::string socket_name = "CANXL_RX_SOCKET";

    if (!canxl.CreateSocket(socket_name)) {
        std::cerr << "CreateSocket failed" << std::endl;
        return 1;
    }

    if (!canxl.setNetworkInterfaceUp(interface_name, socket_name)) {
        std::cerr << "setNetworkInterfaceUp failed for interface: " << interface_name << std::endl;
        return 1;
    }

    canxl.setID(rx_id);
    std::cout << "Listening on interface=" << interface_name
              << " filter_id=0x" << std::hex << rx_id << std::dec << std::endl;

    canxl.ReceiveMessage(socket_name, on_receive);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
