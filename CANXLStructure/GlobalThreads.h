#ifndef GLOBALTHREADS_H
#define GLOBALTHREADS_H

#include <thread>
inline std::jthread backgroundListeningThread{};
inline std::jthread backgroundSendingThread{};

#endif // GLOBALTHREADS_H