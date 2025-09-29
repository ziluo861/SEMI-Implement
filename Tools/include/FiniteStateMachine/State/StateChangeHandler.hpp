#pragma once

#include <FiniteStateMachine/Helper/fsmconcepts.hpp>
#include "FiniteStateMachine/State/IStateChangeHandler.hpp"
#include <functional>

template<typename T>
requires StateIndex<T>
class State;

template<typename TIndex>
class StateChangeHandler : public IStateChangeHandler<TIndex> {
private:
    using StateCallbackType = std::function<void(State<TIndex>&)>;
    StateCallbackType enter_callback_;
    StateCallbackType exit_callback_;
public:
    explicit StateChangeHandler(StateCallbackType enter_callback = {},
    StateCallbackType exit_callback = {}) : enter_callback_(std::move(enter_callback)),
                                       exit_callback_(std::move(exit_callback)){}
    void on_enter(State<TIndex>& holdingstate) override {
        if (enter_callback_) {
            enter_callback_(holdingstate);
        }
    }
    void on_exit(State<TIndex>& holdingstate) override {
        if (exit_callback_) {
            exit_callback_(holdingstate);
        }
    }
};