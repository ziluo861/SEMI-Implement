#pragma once

#include "FiniteStateMachine/State/State.hpp"
#include "Requirements/RequirementMonitor.hpp"
#include <FiniteStateMachine/Transition/Transition.hpp>

template <typename TIndex> class SelfTransition : public Transition<TIndex> {
private:
  State<TIndex> *target_;
  bool enter_history_;
  using BaseType = Transition<TIndex>;
  using CallbackType = typename BaseType::HandlerCallbackType;

public:
  explicit SelfTransition(State<TIndex> &state,
                          RequirementMonitor *requirementmonitor,
                          CallbackType transitioncallback, bool enter_history)
      : BaseType(state, state, requirementmonitor,
                 std::move(transitioncallback), enter_history),
        target_(&state), enter_history_(enter_history) {}

  void Transit() override {
    BaseType::set_blocked(true);

    target_->Exit();
    BaseType::TakeTransitAction();

    // if (enter_history_) {
    //     target_->HistoryState()->Enter();
    //     //target_->Enter();
    // } else {
    //     target_->EntranceState()->Enter();
    // }

    State<TIndex> *enter_target =
        enter_history_ ? target_->HistoryState() : target_->EntranceState();

    enter_target->Enter();

    if (auto *sm = target_->StateMachine()) {
      State<TIndex> *leaf = enter_target;
      while (leaf && !leaf->IsTerminal())
        leaf = leaf->CurrentState();

      if (leaf && leaf->Index() != target_->Index()) {
        sm->NotifyTransition(target_->Index(), leaf->Index());
      }

      sm->UpdateCurrentState();
      sm->FlushPendingTransitions();
    }
  }
};
