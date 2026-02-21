#include "CANXL.h"
#include "CANSec.h"
#include <cstring>
#include <iomanip>
#include <iostream>
#include <span>

void ListeningTry(CANXLStruct myCAN) {
  std::cout << "Geldi geldi" << std::endl;
  std::cout << "ID:" << myCAN.CANID << std::endl;
  std::cout << "Received length: " << static_cast<int>(myCAN.LENGTH)
            << std::endl;

  // Extract ciphertext and tag from received data
  // Format: [ciphertext (variable length)] + [tag (16 bytes)]
  if (myCAN.LENGTH < 16) {
    std::cerr << "Received data too short (need at least 16 bytes for tag)"
              << std::endl;
    return;
  }

  const int ciphertext_len = myCAN.LENGTH - 16;
  std::array<__uint8_t, 32> ciphertext{};
  std::array<__uint8_t, 16> tag{};

  // Copy ciphertext
  std::memcpy(ciphertext.data(), myCAN.DATA, ciphertext_len);
  // Copy tag (last 16 bytes)
  std::memcpy(tag.data(), myCAN.DATA + ciphertext_len, 16);

  std::cout << "Extracted ciphertext length: " << ciphertext_len << std::endl;

  CANSec cansec;
  std::array<__uint8_t, 32> key = {};
  cansec.setKey(key);
  // Set the same nonce as the sender (must match!)
  std::array<__uint8_t, 12> nonce = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                     0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
  cansec.setNonce(nonce);

  std::array<__uint8_t, 32> plaintext{};
  int plaintext_len = 0;

  cansec.DecryptMessage(
      std::span<__uint8_t>(ciphertext.data(), ciphertext_len), ciphertext_len,
      std::span<__uint8_t>(plaintext.data(), plaintext.size()), plaintext_len,
      std::span<__uint8_t>(tag.data(), tag.size()),true);

  std::cout << "Decrypted plaintext length: " << plaintext_len << std::endl;
  std::cout << "Plaintext (hex): ";
  for (int i = 0; i < plaintext_len; i++) {
    std::cout << std::hex << static_cast<int>(plaintext[i]) << " ";
  }
  std::cout << std::dec << std::endl;
}

void ReceiverTestNewMethod(CANXLStruct myCAN) {

  std::cout << "Geldi geldi" << std::endl;
  std::cout << "ID:" << myCAN.CANID << std::endl;
  std::cout << "Received length: " << static_cast<int>(myCAN.LENGTH)
            << std::endl;

  // Extract ciphertext and tag from received data
  // Format: [ciphertext (variable length)] + [tag (16 bytes)]
  if (myCAN.LENGTH < 16) {
    std::cerr << "Received data too short (need at least 16 bytes for tag)"
              << std::endl;
    return;
  }

  int ciphertext_len = myCAN.LENGTH - TAG_LENGTH - COUNTER_LENGTH;
  std::array<__uint8_t, MAX_CIPHERTEXT_LENGTH> ciphertext{};
  std::array<__uint8_t, TAG_LENGTH> tag{};

  memcpy(ciphertext.data(), myCAN.DATA, ciphertext_len);
  memcpy(tag.data(), myCAN.DATA + ciphertext_len, TAG_LENGTH);

  // Check the counter. If the receive counter is smaller than the send counter,
  // the message will be discarded
  int counter = -1;
  memcpy(&counter, myCAN.DATA + ciphertext_len + TAG_LENGTH, COUNTER_LENGTH);

  std::cout << "Counter: " << counter << std::endl;

  myCAN.counter = counter;

  CANSec cansec;
  std::array<__uint8_t, 32> key = {};
  cansec.setKey(key);
  // Set the same nonce as the sender (must match!)
  std::array<__uint8_t, 12> nonce = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                     0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
  cansec.setNonce(nonce);

  std::array<__uint8_t, 32> plaintext{};
  int plaintext_len = 0;

  cansec.DecryptMessage(
      std::span<__uint8_t>(ciphertext.data(), ciphertext_len), ciphertext_len,
      std::span<__uint8_t>(plaintext.data(), plaintext.size()), plaintext_len,
      std::span<__uint8_t>(tag.data(), tag.size()),true);

  std::cout << "Decrypted plaintext length: " << plaintext_len << std::endl;
  std::cout << "Plaintext (hex): ";
  for (int i = 0; i < plaintext_len; i++) {
    std::cout << std::hex << static_cast<int>(plaintext[i]) << " ";
  }
  std::cout << std::dec << std::endl;
}

int main() {
  CANXL my_CANFD;
  my_CANFD.CreateSocket("AleynamSocket");
  my_CANFD.setNetworkInterfaceUp("Aleyna", "AleynamSocket");

  my_CANFD.setID(0x13);

  my_CANFD.ReceiveMessage("AleynamSocket", ReceiverTestNewMethod);

  while (1) {
  }
}