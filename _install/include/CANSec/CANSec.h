#ifndef CANSECKOD_LIBRARY_H
#define CANSECKOD_LIBRARY_H
#include <functional>
#include <array>
#include <span>
#include <mutex>

class CANSec {
public:

    CANSec();
    ~CANSec();

    void sendMessage(const int iID, const bool bIsEncrypted, const unsigned char* cpData, const int iDataLength);
    void setReceivedEvent(std::function<void()> ReceivedFunction);

    /**
     * @brief set the key of the ECUs
     * @param key The key to set
     * @return true if the key was set successfully, false otherwise
     */
    void setKey(const std::array<__uint8_t,32> &key);

    /**
     * @brief get the key of the ECUs
     * @return the current key
     */
    std::array<__uint8_t,32> getKey() ;

    /**
     * @brief set the nonce value (for testing or when nonce needs to be shared)
     * @param nonce The nonce value to set (must be 12 bytes)
     */
    void setNonce(const std::array<__uint8_t,12> &nonce);

    void EncryptMessage(const std::span<__uint8_t> plaintext, const int& plaintext_len, std::span<__uint8_t> ciphertext, int& ciphertext_len,std::span<__uint8_t> tag, bool IsTagUsed) const;

    void DecryptMessage(const std::span<__uint8_t> ciphertext, const int& ciphertext_len, std::span<__uint8_t> plaintext, int& plaintext_len, const std::span<__uint8_t> expected_tag, bool IsTagUsed) const;

private:
    /**
     * @brief Mutex to protect access to the key and nonce
     */
    std::mutex m_mutexKey{};

    std::function<void()> m_funcReceivedFunction; // Callback function for received messages
    void receiveMessage() const ;
    int m_iCounter{0};
    void NonceValueGenerator();
    std::array<__uint8_t,12> m_arrNonceValue{};
    std::array<__uint8_t,32> m_arrKey{};

};
#endif //CANSECKOD_LIBRARY_H