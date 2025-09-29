#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

template <typename TIndex> class Transitable {
private:
  using Self = Transitable<TIndex>;
  using CallbackType = std::function<void(Self &)>;
  std::vector<std::pair<std::size_t, CallbackType>> listeners_;
  bool block_ = true;
  std::size_t id_ = 0;
  std::shared_ptr<bool> alive_;

public:
  class blocksubscribe {
  private:
    void reset() {
      ptr_ = nullptr;
      id_ = 0;
      alive_.reset();
    }
    Transitable *ptr_{nullptr};
    std::size_t id_{0};
    std::weak_ptr<bool> alive_;
    explicit blocksubscribe(Transitable *ptr, std::size_t id,
                            std::weak_ptr<bool> alive) noexcept
        : ptr_(ptr), id_(id), alive_(std::move(alive)) {}
    friend class Transitable<TIndex>;

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
      ptr_->unsubscribe_block_state_change(id_);
      reset();
    }
    ~blocksubscribe() noexcept { release(); }

    blocksubscribe(const blocksubscribe &) = delete;
    blocksubscribe &operator=(const blocksubscribe &) = delete;

    blocksubscribe(blocksubscribe &&obj) noexcept
        : ptr_(std::exchange(obj.ptr_, nullptr)),
          id_(std::exchange(obj.id_, 0)), alive_(std::move(obj.alive_)) {}
    blocksubscribe &operator=(blocksubscribe &&obj) noexcept {
      if (this != &obj) {
        this->release();
        ptr_ = std::exchange(obj.ptr_, nullptr);
        id_ = std::exchange(obj.id_, 0);
        this->alive_ = std::move(obj.alive_);
      }
      return *this;
    }
  };

  Transitable() : alive_(std::make_shared<bool>(true)) {}

  virtual ~Transitable() {
    if (alive_) {
      *alive_ = false;
      alive_.reset();
    }
    listeners_.clear();
  }

  Transitable(const Transitable &) = delete;
  Transitable &operator=(const Transitable &) = delete;
  Transitable(Transitable &&) = delete;
  Transitable &operator=(Transitable &&) = delete;

  bool blocked() const noexcept { return block_; }

  [[nodiscard]]
  blocksubscribe subscribe_block_state_change(CallbackType callback) {
    const auto _size = ++id_;
    listeners_.emplace_back(_size, std::move(callback));
    return blocksubscribe(this, _size, alive_);
  }
  void unsubscribe_block_state_change(std::size_t id) {
    if (!alive_ || !*alive_ || listeners_.empty())
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
      if (!alive_ || !*alive_)
        break;
      callback(*this);
    }
  }
};
