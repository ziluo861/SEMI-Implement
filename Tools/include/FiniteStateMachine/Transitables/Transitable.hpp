#pragma once

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

template <typename TIndex> 
class Transitable {
private:
  using Self = Transitable<TIndex>;
  using CallbackType = std::function<void(Self &)>;
  std::vector<std::pair<std::size_t, CallbackType>> listeners_;
  bool block_ = true;
  bool destroyed_ = false;
  std::size_t id_ = 0;

public:
  class blocksubscribe {
  private:
    void release() noexcept{
      if (ptr_ && !ptr_->destroyed_) {
        ptr_->unsubscribe_block_state_change(id_);
      }
    }
    Transitable *ptr_;
    std::size_t id_;
    explicit blocksubscribe(Transitable *ptr, std::size_t id) noexcept
        : ptr_(ptr), id_(id) {}
    friend class Transitable<TIndex>;
  public:

    ~blocksubscribe() noexcept { release(); }

    blocksubscribe(const blocksubscribe &) = delete;
    blocksubscribe &operator=(const blocksubscribe &) = delete;

    blocksubscribe(blocksubscribe &&obj) noexcept
        : ptr_(std::exchange(obj.ptr_, nullptr)),
          id_(std::exchange(obj.id_, 0)) {}
    blocksubscribe &operator=(blocksubscribe &&obj) noexcept {
      if (this != &obj) {
        this->release();
        ptr_ = std::exchange(obj.ptr_, nullptr);
        id_ = std::exchange(obj.id_, 0);
      }
      return *this;
    }
  };

  Transitable() {}

  virtual ~Transitable() {
    destroyed_ = true;
    listeners_.clear();
  }

  Transitable(const Transitable&) = delete;
  Transitable& operator=(const Transitable&) = delete;
  Transitable(Transitable&&) = delete;
  Transitable& operator=(Transitable &&) = delete;

  bool blocked() const noexcept { return block_; }

  [[nodiscard]]
  blocksubscribe subscribe_block_state_change(CallbackType callback) {
    const auto _size = ++id_;
    listeners_.emplace_back(_size, std::move(callback));
    return blocksubscribe(this, _size);
  }
  void unsubscribe_block_state_change(std::size_t id) {
    if (destroyed_ || listeners_.empty())
      return;
    auto it = std::find_if(listeners_.begin(), listeners_.end(),
                           [id](const auto &p) { return p.first == id; });
    if (it != listeners_.end()) {
      listeners_.erase(it);
    }
  }

  virtual void Transit() = 0;

protected:
  void set_blocked(bool block) {
    if (block_ == block) {
      return;
    }
    block_ = block;
    auto callbacks = listeners_;
    for (auto &[id, callback] : callbacks) {
      if (destroyed_) break;
      callback(*this);
    }
  }
};