#ifndef TIMESTAMP_UTILS_H
#define TIMESTAMP_UTILS_H

#include <cstdint>
#include <string>

struct TimestampParts {
    int year{};
    int month{};
    int day{};
    int hour{};
    int minute{};
    int second{};
    std::uint32_t nanosecond{};
};

std::uint64_t getCurrentRealtimeNanoseconds();
TimestampParts splitTimestampNanoseconds(std::uint64_t timestampNanoseconds);
std::string formatTimestampNanoseconds(std::uint64_t timestampNanoseconds);

#endif
