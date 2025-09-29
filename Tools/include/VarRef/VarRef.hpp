#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <utility>
#include <vector>

template <typename T>
concept EqualityComparable = std::equality_comparable<T>;

template <typename TValue>
  requires std::copy_constructible<TValue> && EqualityComparable<TValue>
class VarRef {
public:
  class ValueSubscribe {
  private:
    void release() {
      if (ptr_ && !ptr_->destroyed_) {
        ptr_->unsubscribe_value_changed(id_);
      }
    }
    VarRef *ptr_;
    std::size_t id_;
    explicit ValueSubscribe(VarRef *ptr, std::size_t id) noexcept
    : ptr_(ptr), id_(id){}
    friend class VarRef<TValue>;
  public:
    ~ValueSubscribe() { release(); }
    ValueSubscribe() : ptr_(nullptr), id_(0) {}
    ValueSubscribe(ValueSubscribe &&obj) noexcept
        : ptr_(std::exchange(obj.ptr_, nullptr)),
          id_(std::exchange(obj.id_, 0)){}
    ValueSubscribe &operator=(ValueSubscribe &&obj) noexcept {
      if (this != &obj) {
        this->release();
        this->ptr_ = std::exchange(obj.ptr_, nullptr);
        this->id_ = std::exchange(obj.id_, 0);
      }
      return *this;
    }

    ValueSubscribe(const ValueSubscribe &) = delete;
    ValueSubscribe &operator=(const ValueSubscribe &) = delete;
  };
  class ObservedStateSubscribe {
  private:
    void release() noexcept {
      if (ptr_ && !ptr_->destroyed_) {
        ptr_->unsubscribe_observed_changed(id_);
      }
    }
    VarRef *ptr_;
    std::size_t id_;
    explicit ObservedStateSubscribe(VarRef *ptr, std::size_t id) noexcept
        : ptr_(ptr), id_(id) {}
    friend class VarRef<TValue>;
  public:

    ObservedStateSubscribe(ObservedStateSubscribe &&obj) noexcept
        : ptr_(std::exchange(obj.ptr_, nullptr)),
          id_(std::exchange(obj.id_, 0)){}
    ObservedStateSubscribe &operator=(ObservedStateSubscribe &&obj) noexcept {
      if (this != &obj) {
        this->release();
        this->ptr_ = std::exchange(obj.ptr_, nullptr);
        this->id_ = std::exchange(obj.id_, 0);
      }
      return *this;
    }

    ObservedStateSubscribe(const ObservedStateSubscribe &) = delete;
    ObservedStateSubscribe &operator=(const ObservedStateSubscribe &) = delete;
    ~ObservedStateSubscribe() { release(); }
  };
  using ValueType = TValue;
  using ComparerType =
      std::function<bool(const ValueType &, const ValueType &)>;
  using ValueChangedCallbackType =
      std::function<void(const VarRef &, const ValueType &, const ValueType &)>;
  using ObservedChangedCallbackType = std::function<void(bool)>;

  virtual ~VarRef() {
    destroyed_ = true;
    Value_Changed_Callbacks_.clear();
    Observed_Changed_Callbacks_.clear();
  }

  explicit VarRef(ValueType defaultValue, ComparerType comparer = nullptr)
      : Default_Value_(defaultValue), Value_(defaultValue),
        comparer_(std::move(comparer)) {}

  VarRef(const VarRef &) = delete;
  VarRef &operator=(const VarRef &) = delete;

  VarRef(VarRef &&) = delete;
  VarRef &operator=(VarRef &&) = delete;

  const ValueType &value() const noexcept { return Value_; }
  const ValueType &default_value() const noexcept { return Default_Value_; }
  bool value_observed() const noexcept { return value_observed_; }
  [[nodiscard]]
  ValueSubscribe subscribe_value_changed(ValueChangedCallbackType callback) {
    const auto id = ++next_value_id;
    Value_Changed_Callbacks_.emplace_back(id, std::move(callback));
    update_observed_state();
    return ValueSubscribe(this, id);
  }
  bool unsubscribe_value_changed(std::size_t id) {
    if (destroyed_ || Value_Changed_Callbacks_.empty()) {
      return false;
    }
    auto it = std::find_if(Value_Changed_Callbacks_.begin(),
                           Value_Changed_Callbacks_.end(),
                           [id](const auto &p) { return p.first == id; });
    if (it != Value_Changed_Callbacks_.end()) {
      Value_Changed_Callbacks_.erase(it);
      update_observed_state();
      return true;
    }
    return false;
  }
  void clear_value_changed() {
    Value_Changed_Callbacks_.clear();
    update_observed_state();
  }

  [[nodiscard]]
  ObservedStateSubscribe subscribe_observed_changed(ObservedChangedCallbackType callback) {
    const auto id = ++next_observed_id;
    Observed_Changed_Callbacks_.emplace_back(id, std::move(callback));
    return ObservedStateSubscribe(this, id);
  }
  bool unsubscribe_observed_changed(std::size_t id) {
    if (destroyed_ || Observed_Changed_Callbacks_.empty()) {
      return false;
    }
    auto it = std::find_if(Observed_Changed_Callbacks_.begin(),
                           Observed_Changed_Callbacks_.end(),
                           [id](const auto &p) { return p.first == id; });
    if (it != Observed_Changed_Callbacks_.end()) {
      Observed_Changed_Callbacks_.erase(it);
      return true;
    }
    return false;
  }
  void clear_observed_state_changed() { Observed_Changed_Callbacks_.clear(); }
  void clear_all_callbacks() {
    clear_value_changed();
    clear_observed_state_changed();
  }

protected:
  void set_value(ValueType newvalue) {
    if (Equals(Value_, newvalue)) {
      return;
    }
    auto oldValue = std::move(Value_);
    Value_ = std::move(newvalue);
    auto callbacks = Value_Changed_Callbacks_;
    for (auto &[id, callback] : callbacks) {
      if (destroyed_) {
        break;
      }
      callback(*this, oldValue, Value_);
    }
  }
  bool Equals(const ValueType &x, const ValueType &y) const {
    return comparer_ ? comparer_(x, y) : (x == y);
  }

private:
  std::vector<std::pair<std::size_t, ValueChangedCallbackType>>
      Value_Changed_Callbacks_;
  std::vector<std::pair<std::size_t, ObservedChangedCallbackType>>
      Observed_Changed_Callbacks_;
  ValueType Default_Value_;
  ValueType Value_;
  ComparerType comparer_;
  bool value_observed_ = false;
  std::size_t next_value_id = 0;
  std::size_t next_observed_id = 0;
  bool destroyed_ = false;
  void update_observed_state() {
    auto new_observed_state = !Value_Changed_Callbacks_.empty();
    if (value_observed_ != new_observed_state) {
      value_observed_ = new_observed_state;
      auto callbacks = Observed_Changed_Callbacks_;
      for (auto &[id, callback] : callbacks) {
        if (destroyed_) {
          break;
        }
        callback(value_observed_);
      }
    }
  }
};