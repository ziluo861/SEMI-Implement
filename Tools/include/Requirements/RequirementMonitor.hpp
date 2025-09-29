#pragma once
#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

class RequirementMonitor {
private:
  using CallbackType = std::function<void(RequirementMonitor &)>;
  std::vector<std::pair<std::size_t, CallbackType>> listeners_;
  int monitorCounter_{0};
  bool fulfilled_{false};
  std::shared_ptr<bool> alive_;
  std::size_t id_{0};

public:
  class fulfillsubscribe {
  private:
    RequirementMonitor *ptr_{nullptr};
    std::size_t id_{0};
    std::weak_ptr<bool> alive_;
    void reset() noexcept {
      ptr_ = nullptr;
      id_ = 0;
      alive_.reset();
    }
    explicit fulfillsubscribe(RequirementMonitor *ptr, std::size_t id,
                              std::weak_ptr<bool> alive) noexcept
        : ptr_(ptr), id_(id), alive_(std::move(alive)) {}
    friend class RequirementMonitor;

  public:
    void release() noexcept {
      if (!ptr_) {
        reset();
        return;
      }
      auto sp = alive_.lock();
      if (!sp || !*sp) {
        reset();
        return;
      }
      ptr_->unsubscribe_fulfilled_state_changed(id_);
      reset();
    }
    fulfillsubscribe() {}
    ~fulfillsubscribe() noexcept { release(); }

    fulfillsubscribe(const fulfillsubscribe &) = delete;
    fulfillsubscribe &operator=(const fulfillsubscribe &) = delete;

    fulfillsubscribe(fulfillsubscribe &&obj) noexcept
        : ptr_(std::exchange(obj.ptr_, nullptr)),
          id_(std::exchange(obj.id_, 0)), alive_(std::move(obj.alive_)) {}
    fulfillsubscribe &operator=(fulfillsubscribe &&obj) noexcept {
      if (this != &obj) {
        ptr_ = std::exchange(obj.ptr_, nullptr);
        id_ = std::exchange(obj.id_, 0);
        alive_ = std::move(obj.alive_);
      }
      return *this;
    }
  };
  RequirementMonitor() : alive_(std::make_shared<bool>(true)) {}
  virtual ~RequirementMonitor() {
    if (alive_) {
      *alive_ = false;
      alive_.reset();
    }
  }

  RequirementMonitor(const RequirementMonitor &) = delete;
  RequirementMonitor &operator=(const RequirementMonitor &) = delete;
  RequirementMonitor(RequirementMonitor &&) = delete;
  RequirementMonitor &operator=(RequirementMonitor &&) = delete;

  bool monitoring() const noexcept { return monitorCounter_ > 0; }
  bool Fulfilled() const noexcept { return fulfilled_; }

  [[nodiscard]]
  fulfillsubscribe subscribe_fulfilled_state_changed(CallbackType callback) {
    const auto _size = ++id_;
    listeners_.emplace_back(_size, std::move(callback));
    return fulfillsubscribe(this, _size, alive_);
  }
  void unsubscribe_fulfilled_state_changed(std::size_t id) {
    if (listeners_.empty())
      return;
    auto it = std::find_if(listeners_.begin(), listeners_.end(),
                           [id](const auto &p) { return p.first == id; });
    if (it != listeners_.end()) {
      listeners_.erase(it);
    }
  }
  void start() {
    if (monitorCounter_++ > 0) {
      return;
    }
    fulfilled_ = false;
    on_start();
  }
  void stop() {
    --monitorCounter_;
    if (monitorCounter_ > 0) {
      return;
    }
    fulfilled_ = false;
    on_stop();
  }

protected:
  void set_fulfilled(bool fulfilled) {
    if (!monitoring() || fulfilled_ == fulfilled) {
      return;
    }
    fulfilled_ = fulfilled;
    auto snap = listeners_;
    for (auto &[_, callback] : snap) {
      if (!alive_ || !*alive_) {
        break;
      }
      callback(*this);
    }
  }
  virtual void on_start() = 0;
  virtual void on_stop() = 0;
};

namespace detail {
class BinaryRequirementMonitor : public RequirementMonitor {
private:
  void OnStateChange() { set_fulfilled(UpdateFulfilledState()); }

protected:
  using ValueType = std::unique_ptr<RequirementMonitor>;
  ValueType left_{nullptr};
  ValueType right_{nullptr};
  typename RequirementMonitor::fulfillsubscribe left_subscribe_{};
  typename RequirementMonitor::fulfillsubscribe right_subscribe_{};
  BinaryRequirementMonitor(ValueType left, ValueType right)
      : left_(std::move(left)), right_(std::move(right)) {
    if (!left_ || !right_) {
      throw std::invalid_argument(
          "BinaryRequirementMonitor: left and right monitors cannot be null");
    }
  }
  virtual bool UpdateFulfilledState() = 0;

  void on_start() override {
    left_->start();
    right_->start();
    left_subscribe_ = left_->subscribe_fulfilled_state_changed(
        [this](RequirementMonitor &) { this->OnStateChange(); });
    right_subscribe_ = right_->subscribe_fulfilled_state_changed(
        [this](RequirementMonitor &) { this->OnStateChange(); });
    set_fulfilled(UpdateFulfilledState());
  }
  void on_stop() override {
    left_subscribe_ = {};
    right_subscribe_ = {};
    left_->stop();
    right_->stop();
  }

public:
  virtual ~BinaryRequirementMonitor() = default;
};
class AndRequirementMonitor : public BinaryRequirementMonitor {
private:
  using ValueType = BinaryRequirementMonitor::ValueType;

protected:
  bool UpdateFulfilledState() override {
    return left_->Fulfilled() && right_->Fulfilled();
  }

public:
  AndRequirementMonitor(ValueType left, ValueType right)
      : BinaryRequirementMonitor(std::move(left), std::move(right)) {}
};
class OrRequirementMonitor : public BinaryRequirementMonitor {
private:
  using ValueType = BinaryRequirementMonitor::ValueType;

protected:
  bool UpdateFulfilledState() override {
    return left_->Fulfilled() || right_->Fulfilled();
  }

public:
  OrRequirementMonitor(ValueType left, ValueType right)
      : BinaryRequirementMonitor(std::move(left), std::move(right)) {}
};
class XorRequirementMonitor : public BinaryRequirementMonitor {
private:
  using ValueType = BinaryRequirementMonitor::ValueType;

protected:
  bool UpdateFulfilledState() override {
    return left_->Fulfilled() != right_->Fulfilled();
  }

public:
  XorRequirementMonitor(ValueType left, ValueType right)
      : BinaryRequirementMonitor(std::move(left), std::move(right)) {}
};

class NotRequirementMonitor : public RequirementMonitor {
private:
  using ValueType = std::unique_ptr<RequirementMonitor>;
  ValueType base_monitor_{nullptr};
  typename RequirementMonitor::fulfillsubscribe base_subscribe_{};
  void OnStateChange() { set_fulfilled(!base_monitor_->Fulfilled()); }

protected:
  void on_start() override {
    base_monitor_->start();
    base_subscribe_ = base_monitor_->subscribe_fulfilled_state_changed(
        [this](RequirementMonitor &) { this->OnStateChange(); });
    set_fulfilled(!base_monitor_->Fulfilled());
  }
  void on_stop() override {
    base_subscribe_ = {};
    base_monitor_->stop();
  }

public:
  NotRequirementMonitor(ValueType base_monitor)
      : base_monitor_(std::move(base_monitor)) {
    if (!base_monitor_) {
      throw std::invalid_argument(
          "NotRequirementMonitor: base monitor cannot be null");
    }
  }
  std::unique_ptr<RequirementMonitor> get_base_monitor() && {
    return std::move(base_monitor_);
  }
};
} // namespace detail

template <typename... Monitors>
std::unique_ptr<RequirementMonitor> make_and(Monitors &&...monitor) {
  static_assert(sizeof...(monitor) >= 2,
                "AND operation requires at least 2 monitors");
  return (std::forward<Monitors>(monitor) & ...);
}

template <typename... Monitors>
std::unique_ptr<RequirementMonitor> make_or(Monitors &&...monitor) {
  static_assert(sizeof...(monitor) >= 2,
                "OR operation requires at least 2 monitors");
  return (std::forward<Monitors>(monitor) | ...);
}

inline std::unique_ptr<RequirementMonitor>
make_not(std::unique_ptr<RequirementMonitor> &&m) {
  if (!m)
    return nullptr;

  if (auto *notp = dynamic_cast<detail::NotRequirementMonitor *>(m.get())) {
    return std::move(*notp).get_base_monitor();
  }

  return std::make_unique<detail::NotRequirementMonitor>(std::move(m));
}

inline std::unique_ptr<RequirementMonitor>
operator&(std::unique_ptr<RequirementMonitor> &&left,
          std::unique_ptr<RequirementMonitor> &&right) {
  if (!left)
    return right;
  if (!right)
    return left;
  return std::make_unique<detail::AndRequirementMonitor>(std::move(left),
                                                         std::move(right));
}

inline std::unique_ptr<RequirementMonitor>
operator|(std::unique_ptr<RequirementMonitor> &&left,
          std::unique_ptr<RequirementMonitor> &&right) {
  if (!left)
    return right;
  if (!right)
    return left;
  return std::make_unique<detail::OrRequirementMonitor>(std::move(left),
                                                        std::move(right));
}

inline std::unique_ptr<RequirementMonitor>
operator^(std::unique_ptr<RequirementMonitor> &&left,
          std::unique_ptr<RequirementMonitor> &&right) {
  if (!left)
    return right;
  if (!right)
    return left;
  return std::make_unique<detail::XorRequirementMonitor>(std::move(left),
                                                         std::move(right));
}

inline std::unique_ptr<RequirementMonitor>
operator!(std::unique_ptr<RequirementMonitor> &&m) {
  return make_not(std::move(m));
}
