#pragma once
#include "FiniteStateMachine/State/IStateChangeHandler.hpp"
#include <FiniteStateMachine/Helper/fsmconcepts.hpp>
#include "FiniteStateMachine/Transitables/Transitable.hpp"
#include "FiniteStateMachine/Transition/Transition.hpp"
#include "VarRef/SourceVarRef.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

template<typename TIndex>
class FSM;

template<typename TIndex>
requires StateIndex<TIndex>
class State {
private:
    using EventCallbackType = std::function<void()>;
    using Self = State<TIndex>;
    std::atomic<bool> exiting_{false};
    using state_handler = std::unique_ptr<IStateChangeHandler<TIndex>>;
    TIndex index_;
    std::unordered_map<TIndex, std::unique_ptr<Self>> child_states_;
    Self *parent_ = nullptr;
    Self *current_state_ = nullptr;
    Self *entrance_state_ = nullptr;
    Self *exitus_state_ = nullptr;
    FSM<TIndex> *statemachine_ = nullptr;
    Self* history_state_ = nullptr;
    bool is_terminal_;
    SourceVarRef<bool> running_{false};
    using subscriptes_ = typename Transitable<TIndex>::blocksubscribe;
public:
    EventCallbackType entered_;
    EventCallbackType exited_;
    std::unique_ptr<Self> entrance_owner;
    std::unique_ptr<Self> exitus_owner;
    state_handler state_change_handler_;
    std::vector<std::unique_ptr<Transition<TIndex>>> transitions_;
    std::vector<std::unique_ptr<subscriptes_>> hold_subscriptes_;
    const TIndex& Index() const noexcept { return index_;}
    const std::unordered_map<TIndex, std::unique_ptr<Self>>& ChildStates() const noexcept {
        return child_states_;
    }
    bool Running() const noexcept {return running_.value();}

    Self *Parent() noexcept { return parent_;}
    const Self *Parent() const noexcept { return parent_;}

    Self *CurrentState() noexcept { return current_state_;}
    const Self *CurrentState() const noexcept { return current_state_;}

    Self *EntranceState() noexcept { return entrance_state_;}
    const Self *EntranceState() const noexcept { return entrance_state_;}

    Self *ExitusState() noexcept { return exitus_state_;}
    const Self *ExitusState() const noexcept { return exitus_state_;}

    Self*       HistoryState()       noexcept { return history_state_ ? history_state_ : entrance_state_; }
    const Self* HistoryState() const noexcept { return history_state_ ? history_state_ : entrance_state_; }

    FSM<TIndex>*       StateMachine()       noexcept { return statemachine_; }
    const FSM<TIndex>* StateMachine() const noexcept { return statemachine_; }
    void UpdateStateMachine(FSM<TIndex>* fsm) {
        statemachine_ = fsm;
        if (exitus_state_) exitus_state_->statemachine_ = fsm;
        if (entrance_state_) entrance_state_->statemachine_ = fsm;
    }


    bool IsTerminal() const noexcept {return is_terminal_;}

    bool is_entrance_state() const noexcept {
        return is_terminal_ && parent_ && this == parent_->entrance_state_;
    }
    bool is_exitus_state() const noexcept {
        return is_terminal_ && parent_ && this == parent_->exitus_state_;
    }
    void Enter() {
        if (running_.value()) return;
        running_.set_value(true);

        if (parent_) {
            parent_->current_state_ = this;
            parent_->Enter();
        }

        for (auto& it : transitions_) {
            it->enter();
        }
        if (state_change_handler_) {
            state_change_handler_->on_enter(*this);
        }
        if (entered_) {
            entered_();
        }
        current_state_->Enter();
    }
    void Exit() {
        if (!running_.value() || exiting_.exchange(true)) return;
        
        if (!IsTerminal() && current_state_) {
            Self* leaf = current_state_;
            while (leaf && !leaf->IsTerminal()) leaf = leaf->CurrentState();
            history_state_ = leaf ? leaf : entrance_state_;
        }

        current_state_->Exit();

        for (auto &it : transitions_) {
            it->exit();
        }

        if (state_change_handler_) {
            state_change_handler_->on_exit(*this);
        }
        if (exited_) {
            exited_();
        }
        running_.set_value(false);
        exiting_.store(false);
    }

    State(TIndex index, std::vector<std::unique_ptr<Self>> child_states = {}) 
        : State(index, state_handler{}, std::move(child_states)) {}

    State(TIndex index, state_handler handler, std::vector<std::unique_ptr<Self>> child_states = {}) 
        : State(index, std::move(handler), std::move(child_states), state_handler{}, state_handler{}){}

    State(TIndex index, state_handler handler, 
        std::vector<std::unique_ptr<Self>> child_states, 
        state_handler entrance_handler,
        state_handler exitus_handler) : index_(index), state_change_handler_(std::move(handler)) {      
        if (child_states.empty()) {
            current_state_ = entrance_state_ = exitus_state_ = this;
            is_terminal_ = true;
        } else {
            for (auto& state : child_states) {
                if (state == nullptr) continue;
                if (state->parent_) {
                    throw std::runtime_error("The Child Being Added Belongs To Another State");
                }
                auto key = state->index_;
                state->parent_ = this;
                auto [it, ok] = child_states_.emplace(key, std::move(state));
                if (!ok) throw std::runtime_error("Duplicate child index");
            }

            entrance_owner = std::make_unique<Self>(index, std::move(entrance_handler));
            entrance_owner->parent_ = this;
            entrance_state_ = entrance_owner.get();
            current_state_ = entrance_state_;

            exitus_owner = std::make_unique<Self>(index, std::move(exitus_handler));
            exitus_owner->parent_ = this;
            exitus_state_ = exitus_owner.get();
            is_terminal_ = false;
        }
    }

    void addtransition(std::unique_ptr<Transition<TIndex>> transition, std::unique_ptr<subscriptes_> subscripte) {
        transitions_.emplace_back(std::move(transition));
        hold_subscriptes_.emplace_back(std::move(subscripte));
    }
};