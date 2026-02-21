#ifndef TIPLER_XL_H
#define TIPLER_XL_H

#include <sys/types.h>

#define MAX_CIPHERTEXT_LENGTH (44)
#define COUNTER_LENGTH (4)
#define TAG_LENGTH (8)

#ifndef CANXL_MAX_DLEN
#define CANXL_MAX_DLEN 2048
#endif

struct CANXLStruct {
    u_int32_t CANID;
    u_int16_t LENGTH;
    u_int8_t FLAGS;
    u_int8_t DATA[CANXL_MAX_DLEN] __attribute__((aligned(8)));
    int counter{};
};

#endif
