#pragma once
#include "FiniteStateMachine/State/IStateChangeHandler.hpp"
#include "FiniteStateMachine/Transitables/StateMachineEnterExitTransitable.hpp"
#include "FiniteStateMachine/Transitables/Transitable.hpp"
#include "FiniteStateMachine/Transition/CrossBranchingTransition.hpp"
#include "FiniteStateMachine/Transition/SelfTransition.hpp"
#include "Requirements/RequirementMonitor.hpp"
#include <FiniteStateMachine/FSM.hpp>
#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <stack>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

template <typename TIndex> class FSM {
private:
  using CallbackType = std::function<void(const TIndex &, const TIndex &)>;
  std::vector<std::pair<std::size_t, CallbackType>> transition_occured_;
  std::size_t id_ = 0;
  bool running_ = false;
  bool transiting_ = false;
  std::unique_ptr<State<TIndex>> root_state_ = nullptr;
  State<TIndex> *current_state_ = nullptr;
  using ExitSub = typename Transitable<TIndex>::blocksubscribe;
  std::unique_ptr<StateMachineEnterExitTransitable<TIndex>>
      fsm_enter_exit_transtions_;
  std::unique_ptr<ExitSub> hold_exit_sub_;
  std::unordered_map<TIndex, State<TIndex> *> all_states_;
  std::unordered_set<Transitable<TIndex> *> padding_unblocked_transitions_;
  void OnBlockStateChange(Transitable<TIndex> *_transitable) {
    if (_transitable->blocked()) {
      padding_unblocked_transitions_.erase(_transitable);
      return;
    }
    padding_unblocked_transitions_.emplace(_transitable);
    TryHandlePaddingTransitions();
  }
  void TryHandlePaddingTransitions() {
    if (transiting_)
      return;

    transiting_ = true;
    while (!padding_unblocked_transitions_.empty()) {
      auto *any = *padding_unblocked_transitions_.begin();
      padding_unblocked_transitions_.erase(any);
      any->Transit();
    }
    transiting_ = false;
  }
  static State<TIndex> *FindLCA(State<TIndex> *departure,
                                State<TIndex> *destination) {
    std::stack<State<TIndex> *> departureStack, destStack;
    for (State<TIndex> *state = departure; state; state = state->Parent()) {
      departureStack.emplace(state);
    }

    for (State<TIndex> *state = destination; state; state = state->Parent()) {
      destStack.emplace(state);
    }

    State<TIndex> *lca = nullptr;

    while (!departureStack.empty() && !destStack.empty()) {
      State<TIndex> *_departstate = departureStack.top();
      departureStack.pop();

      State<TIndex> *_deststate = destStack.top();
      destStack.pop();

      if (_departstate != _deststate)
        break;

      lca = _deststate;
    }
    if (!lca)
      throw std::runtime_error(
          "Departure and destination not in the same tree");
    return lca;
  }
  using state_handler = std::unique_ptr<IStateChangeHandler<TIndex>>;
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
  std::size_t version_ = 0;

public:
  class fsmsubscripte {
  public:
    ~fsmsubscripte() noexcept { release(); }
    fsmsubscripte(const fsmsubscripte &) = delete;
    fsmsubscripte &operator=(const fsmsubscripte &) = delete;

    fsmsubscripte(fsmsubscripte &&obj) noexcept
        : ptr_(std::exchange(obj.ptr_, nullptr)),
          id_(std::exchange(obj.id_, 0)), version_(obj.version_),
          alive_(std::move(obj.alive_)) {}
    fsmsubscripte &operator=(fsmsubscripte &&obj) noexcept {
      if (this != &obj) {
        this->release();
        ptr_ = std::exchange(obj.ptr_, nullptr);
        id_ = std::exchange(obj.id_, 0);
        version_ = obj.version_;
        alive_ = std::move(obj.alive_);
      }
      return *this;
    }
    void release() noexcept {
      if (!ptr_) {
        reset();
        return;
      }
      if (ptr_->version_ != version_) {
        reset();
        return;
      }
      auto sp = alive_.lock();
      if (!sp || !*sp) {
        reset();
        return;
      }
      ptr_->unsubscribe(id_);
      reset();
    }

  private:
    explicit fsmsubscripte(FSM *ptr, std::size_t id,
                           std::weak_ptr<bool> alive) noexcept
        : ptr_(ptr), id_(id), version_(ptr ? ptr->version_ : 0),
          alive_(std::move(alive)) {}
    friend class FSM<TIndex>;
    void reset() {
      ptr_ = nullptr;
      id_ = 0;
      alive_.reset();
    }
    FSM *ptr_;
    std::size_t id_, version_;
    std::weak_ptr<bool> alive_;
  };
  [[nodiscard]]
  fsmsubscripte subscribe(CallbackType callback) {
    const std::size_t _size = ++id_;
    transition_occured_.emplace_back(_size, std::move(callback));
    return fsmsubscripte(this, _size, alive_);
  }
  void unsubscribe(std::size_t id) {
    if (!alive_ || !*alive_ || transition_occured_.empty())
      return;
    auto it =
        std::find_if(transition_occured_.begin(), transition_occured_.end(),
                     [id](const auto &p) { return p.first == id; });
    if (it != transition_occured_.end()) {
      transition_occured_.erase(it);
    }
  }
  explicit FSM(TIndex entrance_index,
               std::vector<std::unique_ptr<State<TIndex>>> states)
      : FSM(entrance_index, state_handler{}, std::move(states)) {}
  explicit FSM(TIndex entrance_index, state_handler handler,
               std::vector<std::unique_ptr<State<TIndex>>> states)
      : version_(1) {
    root_state_ = std::make_unique<State<TIndex>>(
        entrance_index, std::move(handler), std::move(states));
    std::stack<State<TIndex> *> sta;
    sta.push(root_state_.get());
    while (!sta.empty()) {
      State<TIndex> *cur_state = sta.top();
      sta.pop();
      if (cur_state->StateMachine()) {
        throw std::runtime_error(
            "The Child Being Added Belongs To Another Finite State Machine");
      }
      cur_state->UpdateStateMachine(this);
      all_states_.emplace(cur_state->Index(), cur_state);
      // if (!ok) throw std::runtime_error("Duplicate state index in FSM");
      for (auto &[_, state] : cur_state->ChildStates()) {
        sta.push(state.get());
      }
    }
    fsm_enter_exit_transtions_ =
        std::make_unique<StateMachineEnterExitTransitable<TIndex>>(this);
    auto _transtions_subscriptes =
        fsm_enter_exit_transtions_->subscribe_block_state_change(
            [this](Transitable<TIndex> &transitable) {
              OnBlockStateChange(&transitable);
            });
    hold_exit_sub_ =
        std::make_unique<ExitSub>(std::move(_transtions_subscriptes));
  }
  ~FSM() {
    if (alive_) {
      *alive_ = false;
      alive_.reset();
    }
    ++version_;
  }
  State<TIndex> *RootState() noexcept { return root_state_.get(); }
  const State<TIndex> *RootState() const noexcept { return root_state_.get(); }

  State<TIndex> *CurrentState() noexcept { return current_state_; }
  const State<TIndex> *CurrentState() const noexcept { return current_state_; }
  void Start(bool enterHistory = false) {
    if (running_)
      return;
    running_ = true;

    if (root_state_->Running()) {
      fsm_enter_exit_transtions_->Block();
      return;
    }

    fsm_enter_exit_transtions_->enter_history_ = enterHistory;
    fsm_enter_exit_transtions_->is_entering_ = true;
    fsm_enter_exit_transtions_->Unblock();
  }
  void Stop() {
    if (!running_)
      return;
    running_ = false;

    if (!root_state_->Running()) {
      fsm_enter_exit_transtions_->Block();
      return;
    }

    fsm_enter_exit_transtions_->is_entering_ = false;
    fsm_enter_exit_transtions_->Unblock();
  }
  void UpdateCurrentState() {
    State<TIndex> *state = root_state_.get();
    if (!state)
      return;
    while (state && !state->IsTerminal()) {
      state = state->CurrentState();
    }
    if (state->Parent() && (state == state->Parent()->EntranceState() ||
                            state == state->Parent()->ExitusState())) {
      state = state->Parent();
    }
    if (current_state_ == state)
      return;
    auto old_index =
        current_state_ ? current_state_->Index() : root_state_->Index();
    current_state_ = state;
    if (old_index == state->Index())
      return;
    auto callbacks = transition_occured_;
    for (auto &[_, callback] : callbacks) {
      if (!alive_ || !*alive_) {
        break;
      }
      callback(old_index, state->Index());
    }
  }
  void AppendTransition(TIndex departure, TIndex destination,
                        RequirementMonitor *requirement,
                        CallbackType transitionHandler, bool enterHistory) {
    if (running_) {
      throw std::runtime_error(
          "Finite State Machine Cannot Append Transition On Running!");
    }
    std::unique_ptr<Transition<TIndex>> transition;
    std::unique_ptr<typename Transitable<TIndex>::blocksubscribe>
        holdsubscripte;
    if (departure == destination) {
      auto it = all_states_.find(departure);
      if (it == all_states_.end())
        throw std::runtime_error("Not found departure");
      State<TIndex> *state = it->second;
      transition = std::make_unique<SelfTransition<TIndex>>(
          *state, requirement, std::move(transitionHandler), enterHistory);
      auto subscription = transition->subscribe_block_state_change(
          [this](Transitable<TIndex> &transitable) {
            OnBlockStateChange(&transitable);
          });
      holdsubscripte =
          std::make_unique<typename Transitable<TIndex>::blocksubscribe>(
              std::move(subscription));
      state->addtransition(std::move(transition), std::move(holdsubscripte));
      return;
    }
    auto it = all_states_.find(departure);
    if (it == all_states_.end())
      throw std::runtime_error("Not found departure");
    State<TIndex> *depart_state = it->second;

    it = all_states_.find(destination);
    if (it == all_states_.end())
      throw std::runtime_error("Not found destination");
    State<TIndex> *dest_state = it->second;

    State<TIndex> *lca = FindLCA(depart_state, dest_state);
    if (lca == depart_state) {
      depart_state = depart_state->EntranceState();
    } else if (lca == dest_state) {
      dest_state = depart_state->ExitusState();
    }
    transition = std::make_unique<CrossBranchingTransition<TIndex>>(
        *depart_state, *dest_state, *lca, requirement,
        std::move(transitionHandler), enterHistory);
    auto subscription = transition->subscribe_block_state_change(
        [this](Transitable<TIndex> &transitable) {
          OnBlockStateChange(&transitable);
        });
    holdsubscripte =
        std::make_unique<typename Transitable<TIndex>::blocksubscribe>(
            std::move(subscription));
    depart_state->addtransition(std::move(transition),
                                std::move(holdsubscripte));
  }

  void FlushPendingTransitions() { TryHandlePaddingTransitions(); }

  void NotifyTransition(const TIndex &from, const TIndex &to) {
    auto callbacks = transition_occured_;
    for (auto &[_, cb] : callbacks) {
      if (!alive_ || !*alive_)
        break;
      cb(from, to);
    }
  }
};
