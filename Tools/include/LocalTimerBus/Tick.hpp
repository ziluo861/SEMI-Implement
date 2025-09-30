#pragma once

#include <chrono>
#include <cstdint>

struct Tick {
  static std::uint64_t GetTickCount() {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
        .count();
  }
};
