#include "CANSec.h"
#include "openssl/aes.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iostream>
#include <cstring>
#include <utility>
#include <memory>
#include "Tipler.h"

CANSec::CANSec() {
    NonceValueGenerator();
}

CANSec::~CANSec() = default;

void CANSec::setReceivedEvent(std::function<void()> ReceivedFunction) {
    m_funcReceivedFunction = std::move(ReceivedFunction);
}

void CANSec::receiveMessage() const  {
    if (m_funcReceivedFunction) {
        m_funcReceivedFunction();
    }
}

void CANSec::NonceValueGenerator() {
    if ( RAND_bytes(m_arrNonceValue.data(),(12*sizeof(__uint8_t)) ) != 1 ) {
        std::cerr << "Error generating random bytes for nonce." << std::endl;
    }
}

void CANSec::EncryptMessage(const std::span<__uint8_t> plaintext,
                        const int &plaintext_len,
                        std::span<__uint8_t> ciphertext,
                        int& ciphertext_len,
                        std::span<__uint8_t> tag,
                        bool IsTagUsed) const
{
    int len = 0;
    int ciphertext_len_local = 0;

    // Use unique_ptr with custom deleter for automatic cleanup
    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx(
        EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx) {
        std::cerr << "Error creating cipher context." << std::endl;
        return;
    }

    // Initialize encryption operation
    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        std::cerr << "Error initializing encryption." << std::endl;
        return;
    }

    // Set key and nonce
    if (EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, m_arrKey.data(), m_arrNonceValue.data()) != 1) {
        std::cerr << "Error setting key and nonce." << std::endl;
        return;
    }

    // Encrypt plaintext
    if (plaintext_len > 0 && EVP_EncryptUpdate(ctx.get(), ciphertext.data(), &len, plaintext.data(), plaintext_len) != 1) {
            std::cerr << "Error during encryption." << std::endl;
            return;
        }
    ciphertext_len_local += len;

    // Finalize encryption - use ciphertext_len_local (not ciphertext_len) to write after data from EncryptUpdate
    if (EVP_EncryptFinal_ex(ctx.get(), ciphertext.data() + ciphertext_len_local, &len) != 1) {
        std::cerr << "Error finalizing encryption." << std::endl;
        return;
    }

    // Calculate the tag
    if (IsTagUsed && EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, TAG_LENGTH, tag.data()) != 1) {
        std::cerr << "Error getting tag." << std::endl;
        return;
    }

    ciphertext_len_local += len;
    ciphertext_len = ciphertext_len_local;
}

void CANSec::DecryptMessage(const std::span<__uint8_t> ciphertext,
    const int &ciphertext_len,
    std::span<__uint8_t> plaintext,
    int& plaintext_len,
    std::span<__uint8_t> expected_tag,
    bool IsTagUsed) const
{
    int len = 0;
    int plaintext_len_local = 0;

    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx(
    EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx) {
    std::cerr << "Error creating cipher context." << std::endl;
    return;
    }

    // 1) Decrypt init
    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
    std::cerr << "Error initializing decryption." << std::endl;
    return;
    }

    // 2) Key + nonce
    if (EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr,
    m_arrKey.data(), m_arrNonceValue.data()) != 1) {
    std::cerr << "Error setting key and nonce." << std::endl;
    return;
    }

    // (Varsa) AAD burada verilir
    // EVP_DecryptUpdate(ctx.get(), nullptr, &len, aad.data(), aad_len);

    // 3) TAG’i ciphertext’ten ÖNCE ayarla
    if (IsTagUsed && expected_tag.size() != TAG_LENGTH) {
    std::cerr << "Tag size is not correct!" << std::endl;
    return;
    }

    if (IsTagUsed && EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG,
        static_cast<int>(expected_tag.size()),
        expected_tag.data()) != 1) {
    std::cerr << "Error setting tag." << std::endl;
    // İstersen: ERR_print_errors_fp(stderr);
    return;
    }

    // 4) Ciphertext’i çöz
    if (ciphertext_len > 0 &&
    EVP_DecryptUpdate(ctx.get(), plaintext.data(), &len,
    ciphertext.data(), ciphertext_len) != 1) {
    std::cerr << "Error during decryption." << std::endl;
    return;
    }
    plaintext_len_local += len;

    // 5) Final + tag doğrulaması
    if (EVP_DecryptFinal_ex(ctx.get(), plaintext.data() + plaintext_len_local, &len) != 1) {
    std::cerr << "Decryption failed: tag mismatch." << std::endl;
    return;
    }

    plaintext_len_local += len;
    plaintext_len = plaintext_len_local;
}


void CANSec::setKey(const std::array<__uint8_t,32> &key) {
    std::scoped_lock<std::mutex> lock(m_mutexKey);
    m_arrKey = key;
}

std::array<__uint8_t,32> CANSec::getKey() {
    std::scoped_lock<std::mutex> lock(m_mutexKey);
    return m_arrKey;
}

void CANSec::setNonce(const std::array<__uint8_t,12> &nonce) {
    std::scoped_lock<std::mutex> lock(m_mutexKey);
    m_arrNonceValue = nonce;
}