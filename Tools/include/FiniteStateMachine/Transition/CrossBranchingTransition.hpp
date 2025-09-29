#pragma once
#include "FiniteStateMachine/State/State.hpp"
#include "Requirements/RequirementMonitor.hpp"
#include <FiniteStateMachine/Transition/Transition.hpp>

template <typename TIndex>
class CrossBranchingTransition : public Transition<TIndex> {
private:
  using BaseType = Transition<TIndex>;
  using CallbackType = typename BaseType::HandlerCallbackType;
  State<TIndex> *branching_;
  State<TIndex> *entering_destination_;
  State<TIndex> *dest_root_;

public:
  explicit CrossBranchingTransition(State<TIndex> &departure,
                                    State<TIndex> &destination,
                                    State<TIndex> &branching,
                                    RequirementMonitor *requirementmonitor,
                                    CallbackType transitioncallback,
                                    bool enter_history)
      : BaseType(departure, destination, requirementmonitor,
                 std::move(transitioncallback), enter_history),
        branching_(&branching), dest_root_(&destination) {
    if (enter_history) {
      entering_destination_ = &destination;
    } else {
      entering_destination_ = destination.EntranceState();
    }
  }
  void Transit() override {
    BaseType::set_blocked(true);

    branching_->CurrentState()->Exit();
    BaseType::TakeTransitAction();

    State<TIndex> *enter_target = BaseType::enter_history_
                                      ? dest_root_->HistoryState()
                                      : dest_root_->EntranceState();

    enter_target->Enter();
    // entering_destination_->Enter();
    if (auto sm = branching_->StateMachine()) {
      // if (dest_root_->Index() != enter_target->Index()) {
      //     sm->NotifyTransition(dest_root_->Index(), enter_target->Index());
      // }
      sm->UpdateCurrentState();
      sm->FlushPendingTransitions();
    }
  }
  void TakeTransitAction() override {
    if (BaseType::handler_) {
      const auto &from = branching_->CurrentState()->Index();
      const auto &to = entering_destination_->Index();
      BaseType::handler_(from, to);
    }
  }
};
