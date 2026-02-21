#ifndef TIPLER_H
#define TIPLER_H
#include <sys/types.h>

// Define the lengths of the ciphertext, counter, and tag
// Ciphertext length is 44 bytes
// Counter length is 4 bytes
// Tag length is 8 bytes
// Total length is 64 bytes
#define MAX_CIPHERTEXT_LENGTH (44)
#define COUNTER_LENGTH (4)
#define TAG_LENGTH (8)

constexpr int MAX_CANFD_DATA_LEN = 64;

struct CANFDStruct {
    u_int32_t CANID;
    u_int8_t LENGTH;
    u_int8_t FLAGS;
    u_int8_t DATA[MAX_CANFD_DATA_LEN] __attribute__((aligned(8)));
    int counter{};
};
#endif