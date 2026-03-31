#include "LocalTimerBus/TimeDelay.hpp"
#include <utility>

std::uint64_t TimeDelay::TargetTick() const { return target_tick_; }

std::uint64_t TimeDelay::GetID() const { return id_; }

TimeDelay::TimeDelay(Action action, std::stop_token cancel_token,
                     std::uint64_t targettick)
    : action_(std::move(action)), cancel_token_(cancel_token),
      target_tick_(targettick), id_(next_id_++) {}

void TimeDelay::DoAction() {
  if (cancel_token_.stop_requested()) {
    return;
  }
  if (action_) {
    action_();
  }
}
