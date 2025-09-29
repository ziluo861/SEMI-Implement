
#pragma once
#include <FiniteStateMachine/Helper/fsmconcepts.hpp>
#include <FiniteStateMachine/Transitables/Transitable.hpp>
#include <Requirements/RequirementMonitor.hpp>
#include <atomic>
#include <functional>
#include <memory>

template <typename TIndex>
  requires StateIndex<TIndex>
class State;

template <typename TIndex> class Transition : public Transitable<TIndex> {
protected:
  using HandlerCallbackType =
      std::function<void(const TIndex &, const TIndex &)>;
  HandlerCallbackType handler_;

private:
  using BaseType = Transitable<TIndex>;
  State<TIndex> *departure_;
  State<TIndex> *destination_;
  RequirementMonitor *requirement_;
  std::atomic<bool> entered_{false};

  std::unique_ptr<RequirementMonitor::fulfillsubscribe> holdfulfillsubscribe_;

public:
  explicit Transition(State<TIndex> &departure, State<TIndex> &destination,
                      RequirementMonitor *requirement,
                      HandlerCallbackType callback, bool enter_history)
      : departure_(&departure), destination_(&destination),
        requirement_(requirement), handler_(std::move(callback)),
        enter_history_(enter_history) {}
  virtual ~Transition() = default;

  bool enter_history_;
  void enter() {
    if (entered_.exchange(true))
      return;
    if (requirement_) {
      requirement_->start();
      auto subscribe = requirement_->subscribe_fulfilled_state_changed(
          [this](RequirementMonitor &requirement) {
            OnRequirementStateChanged(requirement);
          });
      holdfulfillsubscribe_ =
          std::make_unique<RequirementMonitor::fulfillsubscribe>(
              std::move(subscribe));
      if (requirement_->Fulfilled()) {
        BaseType::set_blocked(false);
      }
    } else {
      BaseType::set_blocked(false);
    }
  }
  void exit() {
    if (!entered_.exchange(false))
      return;
    BaseType::set_blocked(true);
    if (requirement_) {
      holdfulfillsubscribe_.reset();
      requirement_->stop();
    }
  }

protected:
  virtual void OnRequirementStateChanged(RequirementMonitor &requirement) {
    auto value = !requirement.Fulfilled();
    BaseType::set_blocked(value);
  }
  virtual void TakeTransitAction() {
    if (handler_) {
      handler_(departure_->Index(), destination_->Index());
    }
  }
};
