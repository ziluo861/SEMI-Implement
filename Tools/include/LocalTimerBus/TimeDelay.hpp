#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <stop_token>

class TimeDelay {
private:
  using Action = std::function<void()>;
  Action action_;
  std::stop_token cancel_token_;
  std::uint64_t target_tick_;
  std::uint64_t id_;
  static inline std::atomic<std::uint64_t> next_id_{0};

public:
  std::uint64_t TargetTick() const;
  std::uint64_t GetID() const;
  TimeDelay(Action action, std::stop_token cancel_token,
            std::uint64_t targettick);
  void DoAction();
  inline bool operator<(const TimeDelay &obj) const {
    if (this->target_tick_ != obj.target_tick_) {
      return this->target_tick_ > obj.target_tick_;
    }
    return this->id_ > obj.id_;
  }
};
