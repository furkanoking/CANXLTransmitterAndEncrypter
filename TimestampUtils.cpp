#include "TimestampUtils.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

std::uint64_t getCurrentRealtimeNanoseconds() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count()
    );
}

TimestampParts splitTimestampNanoseconds(std::uint64_t timestampNanoseconds) {
    const std::time_t seconds = static_cast<std::time_t>(timestampNanoseconds / 1'000'000'000ULL);
    const std::uint32_t nanoseconds = static_cast<std::uint32_t>(timestampNanoseconds % 1'000'000'000ULL);

    std::tm localTime{};
    localtime_r(&seconds, &localTime);

    return TimestampParts{
        .year = localTime.tm_year + 1900,
        .month = localTime.tm_mon + 1,
        .day = localTime.tm_mday,
        .hour = localTime.tm_hour,
        .minute = localTime.tm_min,
        .second = localTime.tm_sec,
        .nanosecond = nanoseconds
    };
}

std::string formatTimestampNanoseconds(std::uint64_t timestampNanoseconds) {
    const TimestampParts parts = splitTimestampNanoseconds(timestampNanoseconds);

    std::ostringstream stream;
    stream << std::setfill('0')
           << std::setw(4) << parts.year << '-'
           << std::setw(2) << parts.month << '-'
           << std::setw(2) << parts.day << ' '
           << std::setw(2) << parts.hour << ':'
           << std::setw(2) << parts.minute << ':'
           << std::setw(2) << parts.second << '.'
           << std::setw(9) << parts.nanosecond;

    return stream.str();
}
