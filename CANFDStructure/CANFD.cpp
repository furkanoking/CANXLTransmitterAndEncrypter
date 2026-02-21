#include "CANFD.h"
#include <sys/socket.h>
#include <net/if.h>
#include <cstring>
#include <iostream>
#include <linux/can.h>
#include <sys/ioctl.h>
#include <linux/can/raw.h>

#include "GlobalThreads.h"
#include "Tipler.h"
#include "CANSec.h"

CANFD::~CANFD() {
    if (m_threadReceive.joinable()) {
        m_threadReceive.join();
    }

    // Clean up resources if necessary
    std::scoped_lock<std::mutex> lock(m_mutexSocketMap);
    for (const auto& [name,socket] : m_mapSocket) {
        close(socket);
    }

}

bool CANFD::setNetworkInterfaceUp(const std::string &interfaceName, const std::string &portName) {
    // Implementation to set up the network interface
    // Validate interface name length to prevent buffer overflow
    if (interfaceName.length() >= IFNAMSIZ) {
        std::cerr << "Interface name too long (max " << (IFNAMSIZ - 1) << " characters): " << interfaceName << std::endl;
        return false;
    }
    
    ifreq the_ifreq{};
    std::strcpy(the_ifreq.ifr_name,interfaceName.c_str());

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

    try {
        if (int interfaceCheck = ioctl(socket_fd, SIOCGIFINDEX, &the_ifreq); interfaceCheck < 0) {
            std::cerr<<" Interface is not valid"<<std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error checking interface: " << e.what() << std::endl;
        return false;
    }

    //
    sockaddr_can the_sockaddr_can{};
    the_sockaddr_can.can_family = AF_CAN;
    the_sockaddr_can.can_ifindex = the_ifreq.ifr_ifindex;
    std::cout<<"debug. socket:"<<socket_fd<<std::endl;
    if (int error_no = bind(socket_fd, reinterpret_cast<sockaddr *>(&the_sockaddr_can), sizeof(the_sockaddr_can));error_no < 0) {
        std::cerr<<"Error binding socket"<<std::endl;
        std::cerr<<"Error number:"<<error_no<<std::endl;

        int err = errno;
        std::cerr << "bind failed, errno = " << err
                  << " -> " << std::strerror(err) << '\n';
        return false;
    }
    else {
        return true;
    }
}

bool CANFD::CreateSocket(const std::string &socketname) {

    if (const int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW); sock < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return false;
    }

    else {
        // Enable CAN FD supporta
        if (int enable_canfd = 1; setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable_canfd, sizeof(enable_canfd)) < 0) {
            std::cerr << "Error enabling CAN FD support on socket" << std::endl;

            close(sock);
            return false;
        }

        std::scoped_lock<std::mutex> lock(m_mutexSocketMap);
        std::cout<<"Socket created successfully: "<<socketname<<std::endl;
        std::cout<<"Socket ID:"<<sock<<std::endl;
        m_mapSocket[socketname] = sock;
        return true;
    }
}

void CANFD::ReceiveMessage(const std::string &socketname, const std::function<void(CANFDStruct)>& callback) {
    // Pass callback by value to avoid use-after-free: the thread gets its own copy
    // Pass socketname by value for the same reason
    std::jthread ThreadListening(&CANFD::ThreadReceiveMessage, this, socketname, callback);

    // Move the thread to the global thread variable. This will ensure that only one background listening thread is active at any time
    backgroundListeningThread = std::move(ThreadListening);
}

void CANFD::ThreadReceiveMessage(const std::string &socketname, std::function<void( CANFDStruct)> callback) {
    int socket_value{-99};

    {
        std::scoped_lock<std::mutex> lock(m_mutexSocketMap);
        try {
            socket_value = m_mapSocket.at(socketname);
        } catch (const std::exception& e) {
            std::cerr << "Error getting socket value: " << e.what() << std::endl;
            return;
        }
    }

    canfd_frame the_listening_frame{};
    CANFDStruct the_CANFD{};

    while (true) {
        if (int nybets_read = read(socket_value, &the_listening_frame, sizeof(canfd_frame)); nybets_read > 0) {
            if (the_listening_frame.can_id != m_iID) {
                std::cout<<"Receiving message is not suitable ID. It will be not read"<<std::endl;
                continue;
            }

            // We will compare the received counter with the message counter
            // If the received counter is lower than the message counter, the message will be discarded
            // Prevent the replay attacks
            int temp_counter = -1;
            if (the_listening_frame.len > MAX_CANFD_DATA_LEN + TAG_LENGTH + COUNTER_LENGTH) {
                std::cerr<<"Received frame length is invalid. Message will be discarded"<<std::endl;
                continue;
             }

             std::memcpy(&temp_counter, (the_listening_frame.data) + MAX_CIPHERTEXT_LENGTH + TAG_LENGTH, COUNTER_LENGTH);

             if(temp_counter < getCounter()) {
             
                 std::cout<<"Received counter is lower than the message counter. Message will be discarded"<<std::endl;
                 continue;
             }

            the_CANFD.CANID = the_listening_frame.can_id;
            the_CANFD.LENGTH = the_listening_frame.len;
            the_CANFD.FLAGS = the_listening_frame.flags;
            std::memcpy(the_CANFD.DATA, the_listening_frame.data, the_listening_frame.len);


            {
                std::scoped_lock<std::mutex> lock(m_mutexCounter);
                ++counter_message;
            }



            // Call customer callback first (if provided) - allows immediate processing
            if (callback) {
                try {
                    callback(the_CANFD);
                } catch (const std::exception& e) {
                    std::cerr << "Error in customer callback: " << e.what() << std::endl;
                }
            }

            // Add received data to queue with mutex protection (for later retrieval if needed)
            setReceievedData(the_CANFD);
        }
    }
}

void CANFD::setReceievedData(const CANFDStruct &data) {
    std::scoped_lock<std::mutex> lock(m_mutexQueue);
    m_queueReceivedData.push(data);
}


void CANFD::SendMessage(const std::string &socketname, const int ID, const int frame_len, const char* data) {
    // Copy data to avoid dangling pointer: the thread gets its own copy
    std::vector<uint8_t> data_copy(data, data + frame_len); //TODO is it necessary? 
    std::jthread ThreadSending(&CANFD::ThreadSendMessage, this, socketname, ID, frame_len, std::move(data_copy));
    backgroundSendingThread = std::move(ThreadSending);
}

void CANFD::ThreadSendMessage(const std::string &socketname, const int ID, const int frame_len, std::vector<uint8_t> data) {

    {
        // increment the counter of the message. This is used for compare with the received counter of the message
        std::scoped_lock<std::mutex> lock(m_mutexCounter);
        counter_message++;
    }

    int socket_value{-99};
    {
        std::scoped_lock<std::mutex> lock(m_mutexSocketMap);
        
        try {
            socket_value = m_mapSocket.at(socketname);
        } catch (const std::exception& e) {
            std::cerr << "Error getting socket value: " << e.what() << std::endl;
            return;
        }
        
        }
    canfd_frame the_sending_frame{};
    the_sending_frame.can_id = ID;
    the_sending_frame.len = frame_len;
    std::memcpy(the_sending_frame.data, data.data(), frame_len);

    std::cout<<"Trying to send message"<<std::endl;

    if (write(socket_value, &the_sending_frame, sizeof(canfd_frame)) < 0) {
        std::cerr << "Error sending message" << std::endl;
        return;
    }
}

void CANFD::setID(const int& ID) {
    m_iID = ID;
}

void CANFD::ReceivedCallbackfunction(const CANFDStruct &data) {
        std::cout<<"Received message"<<std::endl;

        int ciphertext_len = data.LENGTH - 16;
        std::array<__uint8_t, 32> ciphertext{};
        std::array<__uint8_t, 16> tag{};

        // Copy ciphertext
        std::memcpy(ciphertext.data(), data.DATA, data.LENGTH - 16);
        // Copy tag (last 16 bytes)
        std::memcpy(tag.data(), data.DATA + data.LENGTH - 16, 16);

        CANSec cansec;
        std::array<__uint8_t,32> key = {};
        cansec.setKey(key);
        // Set the same nonce as the sender (must match!)
        std::array<__uint8_t,12> nonce = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
        cansec.setNonce(nonce);

        std::array<__uint8_t,32> plaintext{};
        int plaintext_len = 0;

        cansec.DecryptMessage(
            std::span<__uint8_t>(ciphertext.data(), ciphertext_len),
            ciphertext_len,
            std::span<__uint8_t>(plaintext.data(), plaintext.size()),
            plaintext_len,
            std::span<__uint8_t>(tag.data(), tag.size()),
            true
        );

        std::cout << "Decrypted plaintext length: " << plaintext_len << std::endl;
        std::cout << "Plaintext (hex): ";
        for (int i = 0; i < plaintext_len; i++) {
            std::cout << std::hex << static_cast<int>(plaintext[i]) << " ";
        }
        std::cout << std::dec << std::endl;
}
    
int CANFD::getCounter() {
    std::scoped_lock<std::mutex> lock(m_mutexCounter);
    return counter_message;
}

void CANFD::IncrementCounter() {
    std::scoped_lock<std::mutex> lock(m_mutexCounter);
    ++counter_message;
}