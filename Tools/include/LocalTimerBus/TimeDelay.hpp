#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <stop_token>
#include <utility>
class TimeDelay {
private:
  using Action = std::function<void()>;
  Action action_;
  std::stop_token cancel_token_;
  std::uint64_t target_tick_;
  std::uint64_t id_;
  static inline std::atomic<std::uint64_t> next_id_{0};

public:
  std::uint64_t TargetTick() const { return target_tick_; }
  std::uint64_t GetID() const { return id_; }
  TimeDelay(Action action, std::stop_token cancel_token,
            std::uint64_t targettick)
      : action_(std::move(action)), cancel_token_(std::move(cancel_token)),
        target_tick_(targettick), id_(next_id_++) {}
  void DoAction() {
    if (cancel_token_.stop_requested()) {
      return;
    }
    if (action_) {
      action_();
    }
  }
  bool operator<(const TimeDelay &obj) const {
    if (this->target_tick_ != obj.target_tick_) {
      return this->target_tick_ < obj.target_tick_;
    }
    return this->id_ < obj.id_;
  }
};
