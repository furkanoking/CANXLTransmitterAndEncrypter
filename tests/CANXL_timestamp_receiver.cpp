#include "CANXL.h"
#include "TimestampUtils.h"

#include <chrono>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

static void printFrame(const CANXLStruct& frame) {
    std::cout << "[RECEIVER] id=0x" << std::hex << frame.CANID << std::dec
              << " len=" << frame.LENGTH << " data(hex)=";

    for (int i = 0; i < frame.LENGTH; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(frame.DATA[i]) << ' ';
    }

    std::cout << std::dec << " ascii=\"";
    for (int i = 0; i < frame.LENGTH; ++i) {
        const unsigned char character = frame.DATA[i];
        std::cout << (std::isprint(character) ? static_cast<char>(character) : '.');
    }
    std::cout << '"' << std::endl;
}

int main(int argc, char** argv) {
    const std::string interface_name = (argc > 1) ? argv[1] : "Aleyna";
    const int rx_id = (argc > 2) ? std::stoi(argv[2], nullptr, 0) : 0x13;
    const std::string socket_name = (argc > 3) ? argv[3] : "FurkanSocketRx";

    CANXL canxl;

    if (!canxl.CreateSocket(socket_name)) {
        std::cerr << "[RECEIVER] CreateSocket failed" << std::endl;
        return 1;
    }

    if (!canxl.setNetworkInterfaceUp(interface_name, socket_name)) {
        std::cerr << "[RECEIVER] setNetworkInterfaceUp failed for interface: " << interface_name << std::endl;
        return 1;
    }

    canxl.setID(rx_id);
    canxl.startClockLogger("RECEIVER_CLOCK");

    std::cout << "[RECEIVER] interface=" << interface_name
              << " socket=" << socket_name
              << " rx_id=0x" << std::hex << rx_id << std::dec << std::endl;

    canxl.ReceiveMessage(socket_name, [&canxl](CANXLStruct frame) {
        printFrame(frame);

        if (const auto rxTimestamp = canxl.getLastReceiveTimeNanoseconds(); rxTimestamp.has_value()) {
            std::cout << "[RECEIVER] R_x(ns)=" << *rxTimestamp
                      << " R_x(formatted)=" << formatTimestampNanoseconds(*rxTimestamp)
                      << std::endl;
        } else {
            std::cerr << "[RECEIVER] R_x not available yet" << std::endl;
        }
    });

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
