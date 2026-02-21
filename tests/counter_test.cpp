#include "gtest/gtest.h"
#include "CANSec.h"
#include <iostream>
#include "CANBusCommon.h"
#include "CANXL.h"



int main() {
       CANSec cansec;
       std::array<__uint8_t,32> key = {};
       cansec.setKey(key);
       // Set a fixed nonce so sender and receiver use the same nonce
       std::array<__uint8_t,12> nonce = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
       cansec.setNonce(nonce);

       std::array<__uint8_t,32> plaintext{};
 

       int temp_counter = read_counter(0x12);
       std::array<__uint8_t,sizeof(temp_counter)> my_counter = {};
       my_counter.at(0) = temp_counter;

       std::array<__uint8_t,TAG_LENGTH> tag = {};

       std::cout << "Counter value : " << temp_counter << std::endl;
       std::array<__uint8_t,sizeof(temp_counter)> cipher_counter = {};
       int cipher_counter_len = cipher_counter.size();
       std::cout << "cipher_counter_len size : " << cipher_counter_len<< std::endl;

       cansec.EncryptMessage(my_counter, 1, cipher_counter, cipher_counter_len, tag,true);

       std::cout << "cipher_counter_len : " << cipher_counter_len << std::endl;
       int plaintext_len = 0;

       cansec.DecryptMessage(cipher_counter, cipher_counter_len, plaintext, plaintext_len, tag, true);

       for (int i = 0; i < plaintext_len; i++) {
         std::cout << std::hex << static_cast<int>(plaintext[i]) << " ";
       }
       std::cout << std::dec << std::endl;
}