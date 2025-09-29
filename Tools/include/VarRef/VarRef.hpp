#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <memory>
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
    VarRef *ptr_{nullptr};
    std::size_t id_{0};
    std::weak_ptr<bool> alive_;
    void reset() {
      ptr_ = nullptr;
      id_ = 0;
      alive_.reset();
    }

    explicit ValueSubscribe(VarRef *ptr, std::size_t id,
                            std::weak_ptr<bool> alive) noexcept
        : ptr_(ptr), id_(id), alive_(std::move(alive)) {}
    friend class VarRef<TValue>;

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
      ptr_->unsubscribe_value_changed(id_);
      reset();
    }
    ~ValueSubscribe() { release(); }
    ValueSubscribe() = default;
    ValueSubscribe(ValueSubscribe &&obj) noexcept
        : ptr_(std::exchange(obj.ptr_, nullptr)),
          id_(std::exchange(obj.id_, 0)), alive_(std::move(obj.alive_)) {}
    ValueSubscribe &operator=(ValueSubscribe &&obj) noexcept {
      if (this != &obj) {
        this->release();
        this->ptr_ = std::exchange(obj.ptr_, nullptr);
        this->id_ = std::exchange(obj.id_, 0);
        this->alive_ = std::move(obj.alive_);
      }
      return *this;
    }

    ValueSubscribe(const ValueSubscribe &) = delete;
    ValueSubscribe &operator=(const ValueSubscribe &) = delete;
  };
  class ObservedStateSubscribe {
  private:
    VarRef *ptr_{nullptr};
    std::size_t id_{0};
    std::weak_ptr<bool> alive_;
    explicit ObservedStateSubscribe(VarRef *ptr, std::size_t id,
                                    std::weak_ptr<bool> alive) noexcept
        : ptr_(ptr), id_(id), alive_(std::move(alive)) {}
    friend class VarRef<TValue>;
    void reset() {
      ptr_ = nullptr;
      id_ = 0;
      alive_.reset();
    }

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
      ptr_->unsubscribe_observed_changed(id_);
      reset();
    }
    ObservedStateSubscribe() = default;
    ObservedStateSubscribe(ObservedStateSubscribe &&obj) noexcept
        : ptr_(std::exchange(obj.ptr_, nullptr)),
          id_(std::exchange(obj.id_, 0)), alive_(std::move(obj.alive_)) {}
    ObservedStateSubscribe &operator=(ObservedStateSubscribe &&obj) noexcept {
      if (this != &obj) {
        this->release();
        this->ptr_ = std::exchange(obj.ptr_, nullptr);
        this->id_ = std::exchange(obj.id_, 0);
        this->alive_ = std::move(obj.alive_);
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
    if (alive_) {
      *alive_ = false;
      alive_.reset();
    }
    Value_Changed_Callbacks_.clear();
    Observed_Changed_Callbacks_.clear();
  }

  explicit VarRef(ValueType defaultValue, ComparerType comparer = {},
                  const void *tag = nullptr)
      : Default_Value_(defaultValue), Value_(defaultValue),
        comparer_(std::move(comparer)), alive_(std::make_shared<bool>(true)) {
    if (!comparer_) {
      comparer_id_ = nullptr;
    } else {
      comparer_id_ = tag ? tag : this;
    }
  }
  bool same_comparer(const VarRef<TValue> &obj) const noexcept {
    return (this->comparer_id_ == nullptr && obj.comparer_id_ == nullptr) ||
           (this->comparer_id_ != nullptr &&
            this->comparer_id_ == obj.comparer_id_);
  }
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
    return ValueSubscribe(this, id, alive_);
  }
  bool unsubscribe_value_changed(std::size_t id) {
    if (!alive_ || !*alive_ || Value_Changed_Callbacks_.empty()) {
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
  ObservedStateSubscribe
  subscribe_observed_changed(ObservedChangedCallbackType callback) {
    const auto id = ++next_observed_id;
    Observed_Changed_Callbacks_.emplace_back(id, std::move(callback));
    return ObservedStateSubscribe(this, id, alive_);
  }
  bool unsubscribe_observed_changed(std::size_t id) {
    if (!alive_ || !*alive_ || Observed_Changed_Callbacks_.empty()) {
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
  bool Equals(const ValueType &x, const ValueType &y) const {
    return comparer_ ? comparer_(x, y) : (x == y);
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
      if (!alive_ || !*alive_) {
        break;
      }
      callback(*this, oldValue, Value_);
    }
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
  std::shared_ptr<bool> alive_;
  const void *comparer_id_{nullptr};
  void update_observed_state() {
    auto new_observed_state = !Value_Changed_Callbacks_.empty();
    if (value_observed_ != new_observed_state) {
      value_observed_ = new_observed_state;
      auto callbacks = Observed_Changed_Callbacks_;
      for (auto &[id, callback] : callbacks) {
        if (!alive_ || !*alive_) {
          break;
        }
        callback(value_observed_);
      }
    }
  }
};
