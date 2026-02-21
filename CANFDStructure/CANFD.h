#ifndef CANFD_H
#define CANFD_H
#include <string>
#include <map>
#include <thread>
#include <functional>
#include <optional>
#include <queue>
#include <mutex>
#include <atomic>
#include <vector>
#include "Tipler.h"

class CANFD {
public:
    CANFD() = default;
    CANFD(const CANFD&) = delete;
    CANFD& operator=(const CANFD&) = delete;
    CANFD(CANFD&&) = delete;
    CANFD& operator=(CANFD&&) = delete;


    /**
     *
     * @param interfaceName The name of the network interface to set up.
     * This interface should be created before using this function.
     * @param portName The port name associated with the network interface
     * @return true if the interface was set up successfully, false otherwise
     */
    bool setNetworkInterfaceUp(const std::string &interfaceName, const std::string &portName);


    /**
     *
     * @param socketname The name of the socket to create
     * @return
     */
    bool CreateSocket(const std::string &socketname);

    /**
     *
     * @param socketname The name of the socket to send the message to
     * @param ID The CAN ID of the message to send
     * @param frame_len The length of the frame data
     * @param data Pointer to the data to send
     */
    void SendMessage(const std::string &socketname, const int ID, const int frame_len, const char* data);

    /**
     *
     * @param socketname The name of the socket to receive the message from
     * @param callback The callback function decided by the customer to handle the received message.
     *                 The callback receives the CANFDStruct containing the received data.
     *                 Can be nullptr if no callback is needed (data will still be queued).
     */
    void  ReceiveMessage(const std::string &socketname, const std::function<void( CANFDStruct)>&  callback);

    /**
     *
     * @param socketname The name of the socket to receive the message from
     * @param callback The callback function decided by the customer to handle the received message.
     *                 The callback receives the CANFDStruct containing the received data.
     *                 Can be nullptr if no callback is needed (data will still be queued).
     */
    void ThreadReceiveMessage(const std::string &socketname, std::function<void( CANFDStruct)> callback);


    /**
     *
     * @param data The data to set the received data
     */
    void setReceievedData(const CANFDStruct &data);

    /**
     *
     * @param socketname The name of the socket to send the message to
     * @param ID The ID of the message
     * @param frame_len The length of the message
     * @param data The data to send
     */
    void ThreadSendMessage(const std::string &socketname, const int ID, const int frame_len, std::vector<uint8_t> data);
    ~CANFD();

    /**
     * @brief Set the ID of the CANFD instance
     * @param ID The ID to set
     */
    void setID(const int&);

    /**
     * @brief Callback function to handle the received data
     * @param data The received data
     */
    static void ReceivedCallbackfunction(const CANFDStruct &data);


    /**
     * @brief Get the counter of the message
     * @return The counter of the message
     */
    int getCounter();

    /**
     * @brief Increment the counter of the message
     */
    void IncrementCounter();
    
private:
    /**
     * Map to store socket ID associated with their names
     */
    std::map<std::string, int> m_mapSocket{};

    /**
     * Thread for receiving messages
     */
    std::thread m_threadReceive{};

    std::string m_strSocketName{};

    /**
     * ID of the CANFD instance
     */
    int m_iID{};

    /**
     * Queue to store the received data
     */
    std::queue<CANFDStruct> m_queueReceivedData{};

    /**
     * Mutex to protect the socket map from concurrent access
     */
    mutable std::mutex m_mutexSocketMap{};

    /**
     * Mutex to protect the received data queue from concurrent access
     */
    mutable std::mutex m_mutexQueue{};

    /**
     * Mutex to protect the counter of the message from concurrent access
     */
    std::mutex m_mutexCounter{};

    /**
     * Counter to store the number of messages received
     * This is used for compare with the received counter of the message
     * If the received counter is lower than the counter of the message, the message will be discarded
     */
    int counter_message{0};

};
#endif
