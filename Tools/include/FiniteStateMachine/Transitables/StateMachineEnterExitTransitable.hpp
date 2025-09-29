#pragma once
#include <FiniteStateMachine/Transitables/Transitable.hpp>

template <typename TIndex> class FSM;

template <typename TIndex>
class StateMachineEnterExitTransitable : public Transitable<TIndex> {
private:
  using BaseType = Transitable<TIndex>;

public:
  bool is_entering_{false};
  bool enter_history_{false};
  FSM<TIndex> *fsm_;
  ~StateMachineEnterExitTransitable() = default;

  explicit StateMachineEnterExitTransitable(FSM<TIndex> *fsm) : fsm_(fsm) {}

  void Block() { BaseType::set_blocked(true); }

  void Unblock() { BaseType::set_blocked(false); }

  void Transit() override {
    BaseType::set_blocked(true);

    if (is_entering_) {
      if (enter_history_) {
        fsm_->RootState()->Enter();
      } else {
        fsm_->RootState()->EntranceState()->Enter();
      }
    } else {
      fsm_->RootState()->Exit();
    }

    fsm_->UpdateCurrentState();
    // fsm_->FlushPendingTransitions();
  }
};
