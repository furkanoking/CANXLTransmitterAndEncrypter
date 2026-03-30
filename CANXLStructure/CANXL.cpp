#include "CANXL.h"

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/errqueue.h>
#include <linux/net_tstamp.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#include <cstring>
#include <cerrno>
#include <iostream>
#include <array>
#include <chrono>
#include <span>
#include <cstdint>

#include "CANSec.h"
#include "GlobalThreads.h"
#include "TimestampUtils.h"

namespace {
std::uint64_t timespecToNanoseconds(const timespec& value) {
    return (static_cast<std::uint64_t>(value.tv_sec) * 1'000'000'000ULL) +
           static_cast<std::uint64_t>(value.tv_nsec);
}

std::uint64_t getRealtimeNanosecondsFallback() {
    timespec now{};
    clock_gettime(CLOCK_REALTIME, &now);
    return timespecToNanoseconds(now);
}

std::optional<std::uint64_t> extractTimestampNanoseconds(msghdr& msg) {
    for (cmsghdr* control = CMSG_FIRSTHDR(&msg); control != nullptr; control = CMSG_NXTHDR(&msg, control)) {
        if (control->cmsg_level != SOL_SOCKET || control->cmsg_type != SCM_TIMESTAMPING) {
            continue;
        }

        auto* timestamps = reinterpret_cast<scm_timestamping*>(CMSG_DATA(control));
        for (const timespec& candidate : timestamps->ts) {
            if (candidate.tv_sec != 0 || candidate.tv_nsec != 0) {
                return timespecToNanoseconds(candidate);
            }
        }
    }

    return std::nullopt;
}

void enableSocketTimestamping(int socketFd) {
    const int flags = SOF_TIMESTAMPING_SOFTWARE |
                      SOF_TIMESTAMPING_RX_SOFTWARE |
                      SOF_TIMESTAMPING_TX_SOFTWARE;

    if (setsockopt(socketFd, SOL_SOCKET, SO_TIMESTAMPING, &flags, sizeof(flags)) < 0) {
        std::cerr << "Warning: failed to enable SO_TIMESTAMPING: errno=" << errno
                  << " (" << std::strerror(errno) << ")" << std::endl;
    }
}

void logNamedTimestamp(const char* label, std::uint64_t timestampNanoseconds) {
    std::cout << '[' << label << "] ns=" << timestampNanoseconds
              << " formatted=" << formatTimestampNanoseconds(timestampNanoseconds)
              << std::endl;
}
} // namespace

CANXL::~CANXL() {
    if (m_threadReceive.joinable()) {
        m_threadReceive.join();
    }

    std::scoped_lock<std::mutex> lock(m_mutexSocketMap);
    for (const auto& [name, socket] : m_mapSocket) {
        (void)name;
        close(socket);
    }
}

bool CANXL::setNetworkInterfaceUp(const std::string& interfaceName, const std::string& portName) {
    if (interfaceName.length() >= IFNAMSIZ) {
        std::cerr << "Interface name too long (max " << (IFNAMSIZ - 1) << " characters): " << interfaceName << std::endl;
        return false;
    }

    ifreq the_ifreq{};
    std::strcpy(the_ifreq.ifr_name, interfaceName.c_str());

    int socket_fd = -1;
    {
        std::scoped_lock<std::mutex> lock(m_mutexSocketMap);
        auto it = m_mapSocket.find(portName);
        if (it == m_mapSocket.end()) {
            std::cerr << "Socket not found: " << portName << std::endl;
            return false;
        }
        socket_fd = it->second;
    }

    if (ioctl(socket_fd, SIOCGIFINDEX, &the_ifreq) < 0) {
        std::cerr << "Interface is not valid" << std::endl;
        return false;
    }

    sockaddr_can the_sockaddr_can{};
    the_sockaddr_can.can_family = AF_CAN;
    the_sockaddr_can.can_ifindex = the_ifreq.ifr_ifindex;

    if (bind(socket_fd, reinterpret_cast<sockaddr*>(&the_sockaddr_can), sizeof(the_sockaddr_can)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        return false;
    }

    return true;
}

bool CANXL::CreateSocket(const std::string& socketname) {
    const int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return false;
    }

#ifdef CANXL_MTU
    int enable_canxl = 1;
    if (setsockopt(sock, SOL_CAN_RAW, CAN_RAW_XL_FRAMES, &enable_canxl, sizeof(enable_canxl)) < 0) {
        std::cerr << "Error enabling CAN XL support on socket" << std::endl;
        close(sock);
        return false;
    }
#else
    std::cerr << "Kernel headers do not expose CAN XL support (CANXL_MTU missing)." << std::endl;
    close(sock);
    return false;
#endif

    enableSocketTimestamping(sock);

    std::scoped_lock<std::mutex> lock(m_mutexSocketMap);
    m_mapSocket[socketname] = sock;
    return true;
}

void CANXL::ReceiveMessage(const std::string& socketname, const std::function<void(CANXLStruct)>& callback) {
    std::jthread threadListening(&CANXL::ThreadReceiveMessage, this, socketname, callback);
    backgroundListeningThread = std::move(threadListening);
}

void CANXL::ThreadReceiveMessage(const std::string& socketname, std::function<void(CANXLStruct)> callback) {
    int socket_value{-1};
    {
        std::scoped_lock<std::mutex> lock(m_mutexSocketMap);
        auto it = m_mapSocket.find(socketname);
        if (it == m_mapSocket.end()) {
            std::cerr << "Error getting socket value: " << socketname << std::endl;
            return;
        }
        socket_value = it->second;
    }

#if defined(CANXL_MAX_DLEN)
    CANXLStruct parsed{};

    while (true) {
        canxl_frame frame{};
        std::array<char, CMSG_SPACE(sizeof(scm_timestamping))> control_buffer{};
        iovec io_vector{};
        io_vector.iov_base = &frame;
        io_vector.iov_len = sizeof(frame);

        msghdr message{};
        message.msg_iov = &io_vector;
        message.msg_iovlen = 1;
        message.msg_control = control_buffer.data();
        message.msg_controllen = control_buffer.size();

        const int bytes_read = static_cast<int>(recvmsg(socket_value, &message, 0));
        if (bytes_read <= 0) {
            if (bytes_read < 0 && errno != EINTR) {
                std::cerr << "[CANXL][RX] recvmsg failed: errno=" << errno
                          << " (" << std::strerror(errno) << ")" << std::endl;
            }
            continue;
        }

        if (const auto timestamp = extractTimestampNanoseconds(message); timestamp.has_value()) {
            m_lastReceiveTimeNanoseconds.store(*timestamp, std::memory_order_relaxed);
            logNamedTimestamp("R_x", *timestamp);
        } else {
            const std::uint64_t fallbackTimestamp = getRealtimeNanosecondsFallback();
            std::cerr << "[CANXL][RX] kernel timestamp missing, falling back to CLOCK_REALTIME" << std::endl;
            m_lastReceiveTimeNanoseconds.store(fallbackTimestamp, std::memory_order_relaxed);
            logNamedTimestamp("R_x", fallbackTimestamp);
        }

#ifdef CANXL_XLF
        if ((frame.flags & CANXL_XLF) == 0U) {
            continue;
        }
#endif
        if (frame.prio != static_cast<canid_t>(m_iID)) {
            continue;
        }
        if (frame.len > CANXL_MAX_DLEN) {
            std::cerr << "Received CAN XL length is invalid" << std::endl;
            continue;
        }

        parsed.CANID = frame.prio;
        parsed.LENGTH = frame.len;
        parsed.FLAGS = frame.flags;
        std::memcpy(parsed.DATA, frame.data, frame.len);

        {
            std::scoped_lock<std::mutex> lock(m_mutexCounter);
            ++counter_message;
        }

        if (callback) {
            try {
                callback(parsed);
            } catch (const std::exception& e) {
                std::cerr << "Error in customer callback: " << e.what() << std::endl;
            }
        }

        setReceievedData(parsed);
    }
#else
    (void)callback;
    std::cerr << "CAN XL types are not available in linux headers." << std::endl;
#endif
}

void CANXL::setReceievedData(const CANXLStruct& data) {
    std::scoped_lock<std::mutex> lock(m_mutexQueue);
    m_queueReceivedData.push(data);
}

void CANXL::SendMessage(const std::string& socketname, int ID, int frame_len, const char* data) {
    std::vector<uint8_t> data_copy(data, data + frame_len);
    std::jthread threadSending(&CANXL::ThreadSendMessage, this, socketname, ID, frame_len, std::move(data_copy));
    backgroundSendingThread = std::move(threadSending);
}

void CANXL::ThreadSendMessage(const std::string& socketname, int ID, int frame_len, std::vector<uint8_t> data) {
    {
        std::scoped_lock<std::mutex> lock(m_mutexCounter);
        ++counter_message;
    }

    int socket_value{-1};
    {
        std::scoped_lock<std::mutex> lock(m_mutexSocketMap);
        auto it = m_mapSocket.find(socketname);
        if (it == m_mapSocket.end()) {
            std::cerr << "Error getting socket value: " << socketname << std::endl;
            return;
        }
        socket_value = it->second;
    }

#if defined(CANXL_MAX_DLEN)
    if (frame_len < 0 || frame_len > CANXL_MAX_DLEN) {
        std::cerr << "Invalid CAN XL payload length: " << frame_len << std::endl;
        return;
    }

    canxl_frame frame{};
    frame.prio = static_cast<canid_t>(ID);
#ifdef CANXL_XLF
    frame.flags = CANXL_XLF;
#else
    frame.flags = 0;
#endif
    frame.len = static_cast<__u16>(frame_len);
    std::memcpy(frame.data, data.data(), static_cast<size_t>(frame_len));
    const size_t wire_len = static_cast<size_t>(CANXL_HDR_SIZE) + static_cast<size_t>(frame_len);

    if (write(socket_value, &frame, wire_len) < 0) {
        std::cerr << "Error sending CAN XL message: errno=" << errno
                  << " (" << std::strerror(errno) << ")" << std::endl;
        return;
    }

    std::array<char, CMSG_SPACE(sizeof(scm_timestamping))> control_buffer{};
    std::array<char, 1> payload_buffer{};
    iovec io_vector{};
    io_vector.iov_base = payload_buffer.data();
    io_vector.iov_len = payload_buffer.size();

    msghdr message{};
    message.msg_iov = &io_vector;
    message.msg_iovlen = 1;
    message.msg_control = control_buffer.data();
    message.msg_controllen = control_buffer.size();

    if (recvmsg(socket_value, &message, MSG_ERRQUEUE | MSG_DONTWAIT) >= 0) {
        if (const auto timestamp = extractTimestampNanoseconds(message); timestamp.has_value()) {
            m_lastTransmitTimeNanoseconds.store(*timestamp, std::memory_order_relaxed);
            logNamedTimestamp("T_x", *timestamp);
            return;
        }
    }

    const std::uint64_t fallbackTimestamp = getRealtimeNanosecondsFallback();
    std::cerr << "[CANXL][TX] kernel TX timestamp unavailable, falling back to CLOCK_REALTIME" << std::endl;
    m_lastTransmitTimeNanoseconds.store(fallbackTimestamp, std::memory_order_relaxed);
    logNamedTimestamp("T_x", fallbackTimestamp);
#else
    (void)ID;
    (void)frame_len;
    (void)data;
    std::cerr << "CAN XL types are not available in linux headers." << std::endl;
#endif
}

void CANXL::setID(const int& ID) {
    m_iID = ID;
}

void CANXL::ReceivedCallbackfunction(const CANXLStruct& data) {
    std::cout << "Received CAN XL message. len=" << data.LENGTH << std::endl;

    if (data.LENGTH < TAG_LENGTH) {
        return;
    }

    const int ciphertext_len = static_cast<int>(data.LENGTH) - TAG_LENGTH;
    std::array<__uint8_t, 64> ciphertext{};
    std::array<__uint8_t, TAG_LENGTH> tag{};

    if (ciphertext_len > static_cast<int>(ciphertext.size())) {
        return;
    }

    std::memcpy(ciphertext.data(), data.DATA, static_cast<size_t>(ciphertext_len));
    std::memcpy(tag.data(), data.DATA + ciphertext_len, TAG_LENGTH);

    CANSec cansec;
    std::array<__uint8_t, 32> key = {};
    cansec.setKey(key);
    std::array<__uint8_t, 12> nonce = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
    cansec.setNonce(nonce);

    std::array<__uint8_t, 64> plaintext{};
    int plaintext_len = 0;

    cansec.DecryptMessage(std::span<__uint8_t>(ciphertext.data(), static_cast<size_t>(ciphertext_len)),
                          ciphertext_len,
                          std::span<__uint8_t>(plaintext.data(), plaintext.size()),
                          plaintext_len,
                          std::span<__uint8_t>(tag.data(), tag.size()),
                          true);
}

int CANXL::getCounter() {
    std::scoped_lock<std::mutex> lock(m_mutexCounter);
    return counter_message;
}

void CANXL::IncrementCounter() {
    std::scoped_lock<std::mutex> lock(m_mutexCounter);
    ++counter_message;
}

std::optional<std::uint64_t> CANXL::getLastTransmitTimeNanoseconds() const {
    const std::uint64_t timestamp = m_lastTransmitTimeNanoseconds.load(std::memory_order_relaxed);
    if (timestamp == 0) {
        return std::nullopt;
    }

    return timestamp;
}

std::optional<std::uint64_t> CANXL::getLastReceiveTimeNanoseconds() const {
    const std::uint64_t timestamp = m_lastReceiveTimeNanoseconds.load(std::memory_order_relaxed);
    if (timestamp == 0) {
        return std::nullopt;
    }

    return timestamp;
}

void CANXL::startClockLogger(const std::string& label) {
    stopClockLogger();
    m_clockLoggerThread = std::jthread([this, label](std::stop_token stopToken) {
        clockLoggerLoop(stopToken, label);
    });
}

void CANXL::stopClockLogger() {
    if (m_clockLoggerThread.joinable()) {
        m_clockLoggerThread.request_stop();
        m_clockLoggerThread.join();
    }
}

void CANXL::clockLoggerLoop(std::stop_token stopToken, std::string label) {
    while (!stopToken.stop_requested()) {
        const std::uint64_t nowNanoseconds = getCurrentRealtimeNanoseconds();
        std::cout << '[' << label << "] now_ns=" << nowNanoseconds
                  << " formatted=" << formatTimestampNanoseconds(nowNanoseconds)
                  << std::endl;

        for (int i = 0; i < 50 && !stopToken.stop_requested(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
