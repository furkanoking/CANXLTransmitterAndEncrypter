#include "CANXL.h"
#include "TimestampUtils.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char** argv) {
    const std::string interface_name = (argc > 1) ? argv[1] : "Aleyna";
    const int tx_id = (argc > 2) ? std::stoi(argv[2], nullptr, 0) : 0x13;
    const std::string message = (argc > 3) ? argv[3] : "timestamp-demo-from-sender";
    const std::string socket_name = (argc > 4) ? argv[4] : "FurkanSocketTx";

    CANXL canxl;

    if (!canxl.CreateSocket(socket_name)) {
        std::cerr << "[SENDER] CreateSocket failed" << std::endl;
        return 1;
    }

    if (!canxl.setNetworkInterfaceUp(interface_name, socket_name)) {
        std::cerr << "[SENDER] setNetworkInterfaceUp failed for interface: " << interface_name << std::endl;
        return 1;
    }

    canxl.startClockLogger("SENDER_CLOCK");

    std::cout << "[SENDER] interface=" << interface_name
              << " socket=" << socket_name
              << " tx_id=0x" << std::hex << tx_id << std::dec
              << " payload=\"" << message << "\"" << std::endl;

    while (true) {
        canxl.SendMessage(socket_name, tx_id, static_cast<int>(message.size()), message.data());
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        if (const auto txTimestamp = canxl.getLastTransmitTimeNanoseconds(); txTimestamp.has_value()) {
            std::cout << "[SENDER] T_x(ns)=" << *txTimestamp
                      << " T_x(formatted)=" << formatTimestampNanoseconds(*txTimestamp)
                      << std::endl;
        } else {
            std::cerr << "[SENDER] T_x not available yet" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
