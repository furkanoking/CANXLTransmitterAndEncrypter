#ifndef CANXL_H
#define CANXL_H

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <queue>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "TiplerXL.h"

class CANXL {
public:
    CANXL() = default;
    CANXL(const CANXL&) = delete;
    CANXL& operator=(const CANXL&) = delete;
    CANXL(CANXL&&) = delete;
    CANXL& operator=(CANXL&&) = delete;

    bool setNetworkInterfaceUp(const std::string& interfaceName, const std::string& portName);
    bool CreateSocket(const std::string& socketname);
    void SendMessage(const std::string& socketname, int ID, int frame_len, const char* data);
    void ReceiveMessage(const std::string& socketname, const std::function<void(CANXLStruct)>& callback);
    void ThreadReceiveMessage(const std::string& socketname, std::function<void(CANXLStruct)> callback);
    void setReceievedData(const CANXLStruct& data);
    void ThreadSendMessage(const std::string& socketname, int ID, int frame_len, std::vector<uint8_t> data);
    ~CANXL();

    void setID(const int& ID);
    static void ReceivedCallbackfunction(const CANXLStruct& data);
    int getCounter();
    void IncrementCounter();
    std::optional<std::uint64_t> getLastTransmitTimeNanoseconds() const;
    std::optional<std::uint64_t> getLastReceiveTimeNanoseconds() const;
    void startClockLogger(const std::string& label = "CANXL_CLOCK");
    void stopClockLogger();

private:
    void clockLoggerLoop(std::stop_token stopToken, std::string label);
    std::map<std::string, int> m_mapSocket{};
    std::thread m_threadReceive{};
    std::string m_strSocketName{};
    int m_iID{};
    std::queue<CANXLStruct> m_queueReceivedData{};
    mutable std::mutex m_mutexSocketMap{};
    mutable std::mutex m_mutexQueue{};
    std::mutex m_mutexCounter{};
    std::atomic<std::uint64_t> m_lastTransmitTimeNanoseconds{0};
    std::atomic<std::uint64_t> m_lastReceiveTimeNanoseconds{0};
    std::jthread m_clockLoggerThread{};
    int counter_message{0};
};

#endif
