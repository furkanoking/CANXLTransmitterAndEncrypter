#include "CANXL.h"
#include <iostream>
#include "CANSec.h"
#include <cstring>
#include "CANBusCommon.h"


void ListeningTry(CANXLStruct myCAN) {
    std::cout<<"Geldi geldi"<<std::endl;
    std::cout<<"ID:"<<myCAN.CANID<<std::endl;
}

//Test-1 example
/*
int main() {
    CANXL my_CANFD;
    my_CANFD.CreateSocket("FurkanSocket");
    my_CANFD.setNetworkInterfaceUp("Aleyna","FurkanSocket");

    CANSec cansec;
    std::array<__uint8_t,32> key = {};
    cansec.setKey(key);
    // Set a fixed nonce so sender and receiver use the same nonce
    std::array<__uint8_t,12> nonce = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
    cansec.setNonce(nonce);

    std::array<__uint8_t,32> plaintext{};
    plaintext.at(0) = 0xDE;
    plaintext.at(1) = 0xAD;
    std::array<__uint8_t,32> ciphertext = {};
    std::array<__uint8_t,16> tag = {};
    int tem_cipher_len = ciphertext.size();

    // need to tag updated so that we can send the tag to the receiver
    cansec.EncryptMessage(plaintext, 2, ciphertext, tem_cipher_len, tag);

    // Combine ciphertext + tag for sending (ciphertext_len bytes + 16 bytes tag)
    std::array<__uint8_t, 48> data_to_send{};
    std::memcpy(data_to_send.data(), ciphertext.data(), tem_cipher_len);
    std::memcpy(data_to_send.data() + tem_cipher_len, tag.data(), 16);
    int total_length = tem_cipher_len + 16;

    // TODO we need to send the counter of the message to the receiver

    std::cout << "Sending ciphertext length: " << tem_cipher_len << std::endl;
    std::cout << "Total length with tag: " << total_length << std::endl;

    my_CANFD.setID(0x12);
    my_CANFD.SendMessage("FurkanSocket", 0x13, total_length, reinterpret_cast<char*>(data_to_send.data()));
    my_CANFD.ReceiveMessage("FurkanSocket",ListeningTry);

    while (1) {

    }



}

*/

//Test-2 example

int main() {
    CANXL my_CANFD;
    my_CANFD.CreateSocket("FurkanSocket");
    my_CANFD.setNetworkInterfaceUp("Aleyna","FurkanSocket");

    CANSec cansec;
    std::array<__uint8_t,32> key = {};
    cansec.setKey(key);
    // Set a fixed nonce so sender and receiver use the same nonce
    std::array<__uint8_t,12> nonce = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
    cansec.setNonce(nonce);

    std::array<__uint8_t,32> plaintext{};
    plaintext.at(0) = 0xDE;
    plaintext.at(1) = 0xAD;

    std::array<__uint8_t,MAX_CIPHERTEXT_LENGTH> ciphertext = {};
    std::array<__uint8_t,TAG_LENGTH> tag = {};
    int tem_cipher_len = ciphertext.size();
    cansec.EncryptMessage(plaintext, 2, ciphertext, tem_cipher_len, tag, true);

    int temp_counter = read_counter(0x12);
    std::array<__uint8_t,1>array_counter = {};
    array_counter.at(0) = temp_counter;
    
    std::array<__uint8_t,sizeof(temp_counter)> cipher_counter = {};
    int cipher_counter_len = cipher_counter.size();
    cansec.EncryptMessage(array_counter, 1, cipher_counter, cipher_counter_len, tag,false);

    std::array<__uint8_t,MAX_CIPHERTEXT_LENGTH + TAG_LENGTH + COUNTER_LENGTH> data_to_send{};
    memcpy(data_to_send.data(), ciphertext.data(), tem_cipher_len);
    memcpy(data_to_send.data() + tem_cipher_len, tag.data(), TAG_LENGTH);
    memcpy(data_to_send.data() + tem_cipher_len + TAG_LENGTH,cipher_counter.data(), cipher_counter_len);

    int total_length = tem_cipher_len + TAG_LENGTH + COUNTER_LENGTH;
    std::cout << "Sending ciphertext length: " << tem_cipher_len << std::endl;
    std::cout << "Sending total length: " << total_length << std::endl;
    my_CANFD.setID(0x12);
    my_CANFD.SendMessage("FurkanSocket", 0x13, total_length, reinterpret_cast<char*>(data_to_send.data()));
    my_CANFD.ReceiveMessage("FurkanSocket",ListeningTry);

    while (1) {

    }
}