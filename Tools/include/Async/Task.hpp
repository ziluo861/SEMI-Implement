#pragma once
#include <atomic>
#include <coroutine>
#include <exception>
#include <stdexcept>
#include <utility>

template <typename T> struct Task {
  struct promise_type {
    T value_;
    std::exception_ptr exception_{};
    struct alignas(64) WaitingNode {
      std::coroutine_handle<> node_{nullptr};
      WaitingNode *next_{nullptr};
      WaitingNode() = default;
      WaitingNode(const WaitingNode &) = delete;
      WaitingNode &operator=(const WaitingNode &) = delete;
    };
    static WaitingNode *instance() {
      static WaitingNode dummy;
      return &dummy;
    }
    alignas(64) std::atomic<WaitingNode *> cache_{nullptr};
    alignas(64) std::atomic<std::size_t> ref_cnt{1};
    void add_ref() noexcept { ref_cnt.fetch_add(1, std::memory_order_acq_rel); }
    bool release_ref_and_is_last() {
      auto _last = ref_cnt.fetch_sub(1, std::memory_order_acq_rel);
      return _last == 1;
    }
    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_never initial_suspend() const noexcept { return {}; }
    struct Final_Awaiter {
      bool await_ready() const noexcept { return false; }
      std::coroutine_handle<>
      await_suspend(std::coroutine_handle<promise_type> h) noexcept {
        auto &handle = h.promise();
        WaitingNode *head =
            handle.cache_.exchange(instance(), std::memory_order_acq_rel);
        handle.cache_.notify_all();
        if (head && head != instance() && head->node_) {
          return head->node_;
        }
        return std::noop_coroutine();
      }
      void await_resume() noexcept {}
    };
    Final_Awaiter final_suspend() noexcept { return {}; }
    void return_value(T val) { value_ = std::move(val); }
    void unhandled_exception() { exception_ = std::current_exception(); }
  };
  explicit Task(std::coroutine_handle<promise_type> handle) : handle_(handle) {}
  ~Task() {
    if (this->handle_) {
      auto &p = this->handle_.promise();
      if (p.release_ref_and_is_last()) {
        if (p.cache_.load(std::memory_order_acquire) ==
            promise_type::instance()) {
          this->handle_.destroy();
        }
      }
    }
  }

  Task(const Task &) = delete;
  Task &operator=(const Task &) = delete;

  Task(Task &&obj) noexcept : handle_(std::exchange(obj.handle_, {})) {}
  Task &operator=(Task &&obj) noexcept {
    if (this != &obj) {
      if (this->handle_) {
        auto &p = this->handle_.promise();
        if (p.release_ref_and_is_last()) {
          if (p.cache_.load(std::memory_order_acquire) ==
              promise_type::instance()) {
            this->handle_.destroy();
          }
        }
      }
      this->handle_ = std::exchange(obj.handle_, {});
    }
    return *this;
  }
  struct Awaiter {
    Task &self;
    bool is_add_{false};
    explicit Awaiter(Task &task) : self(task) {}
    typename promise_type::WaitingNode Node{};
    bool await_ready() const noexcept {
      return self.handle_.promise().cache_.load(std::memory_order_acquire) ==
             promise_type::instance();
    }
    T await_resume() {
      auto &p = self.handle_.promise();
      auto *next = Node.next_;
      if (p.exception_) {
        if (next && next->node_)
          next->node_.resume();
        if (is_add_) {
          p.release_ref_and_is_last();
        }
        std::rethrow_exception(p.exception_);
      }
      T result = p.value_;
      if (next && next->node_)
        next->node_.resume();
      if (is_add_) {
        p.release_ref_and_is_last();
      }
      return result;
    }
    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> awaiting) noexcept {
      auto &p = self.handle_.promise();

      auto &head = p.cache_;
      auto *head_node = head.load(std::memory_order_acquire);
      Node.node_ = awaiting;
      for (;;) {
        if (head_node == promise_type::instance())
          return awaiting;
        Node.next_ = head_node;
        if (head.compare_exchange_weak(head_node, &Node,
                                       std::memory_order_acq_rel,
                                       std::memory_order_acquire)) {
          p.add_ref();
          is_add_ = true;
          return std::noop_coroutine();
        }
      }
    }
  };

  struct R_Awaiter {
    std::coroutine_handle<promise_type> handle_;
    bool is_add_{false};
    explicit R_Awaiter(Task &&task)
        : handle_(std::exchange(task.handle_, {})) {}
    // 由于外部Task的所有权转移至此，外面Task的析构函数无法工作，相关资源释放转移至此处理
    ~R_Awaiter() {
      auto &p = handle_.promise();
      const bool is_last = p.release_ref_and_is_last();
      if (is_last && p.cache_.load(std::memory_order_acquire) ==
                         promise_type::instance()) {
        handle_.destroy();
      }
    }
    typename promise_type::WaitingNode Node{};
    bool await_ready() const noexcept {
      return handle_.promise().cache_.load(std::memory_order_acquire) ==
             promise_type::instance();
    }
    T await_resume() {
      auto &p = handle_.promise();
      auto *next = Node.next_;
      if (p.exception_) {
        if (next && next->node_)
          next->node_.resume();
        if (is_add_) {
          p.release_ref_and_is_last();
        }
        std::rethrow_exception(p.exception_);
      }
      T result = std::move(p.value_);
      if (next && next->node_)
        next->node_.resume();
      if (is_add_) {
        p.release_ref_and_is_last();
      }

      return result;
    }
    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> awaiting) noexcept {
      auto &p = handle_.promise();
      auto &head = p.cache_;
      auto *head_node = head.load(std::memory_order_acquire);

      Node.node_ = awaiting;
      for (;;) {
        if (head_node == promise_type::instance())
          return awaiting;
        Node.next_ = head_node;
        if (head.compare_exchange_weak(head_node, &Node,
                                       std::memory_order_acq_rel,
                                       std::memory_order_acquire)) {
          p.add_ref();
          is_add_ = true;
          return std::noop_coroutine();
        }
      }
    }
  };

  bool is_ready() const noexcept {
    if (!handle_)
      return true;
    auto &p = handle_.promise();
    return p.cache_.load(std::memory_order_acquire) == promise_type::instance();
  }

  T get() & {
    if (!handle_)
      throw std::logic_error("Task used after move");
    auto &p = handle_.promise();
    if (p.cache_.load(std::memory_order_acquire) != promise_type::instance()) {
      auto expected = p.cache_.load(std::memory_order_acquire);
      while (expected != promise_type::instance()) {
        p.cache_.wait(expected, std::memory_order_acquire);
        expected = p.cache_.load(std::memory_order_acquire);
      }
    }
    if (p.exception_) {
      std::rethrow_exception(p.exception_);
    }
    return p.value_;
  }

  T get() && {
    if (!handle_)
      throw std::logic_error("Task used after move");
    auto &p = handle_.promise();
    if (p.cache_.load(std::memory_order_acquire) != promise_type::instance()) {
      auto expected = p.cache_.load(std::memory_order_acquire);
      while (expected != promise_type::instance()) {
        p.cache_.wait(expected, std::memory_order_acquire);
        expected = p.cache_.load(std::memory_order_acquire);
      }
    }
    if (p.exception_) {
      std::rethrow_exception(p.exception_);
    }
    return std::move(p.value_);
  }

  T get_blocking() {
    if (!handle_)
      throw std::logic_error("Task used after move");
    auto &p = handle_.promise();

    if (p.cache_.load(std::memory_order_acquire) == promise_type::instance()) {
      if (p.exception_) {
        std::rethrow_exception(p.exception_);
      }
      return p.value_;
    }

    auto expected = p.cache_.load(std::memory_order_acquire);
    while (expected != promise_type::instance()) {
      p.cache_.wait(expected, std::memory_order_acquire);
      expected = p.cache_.load(std::memory_order_acquire);
    }

    if (p.exception_) {
      std::rethrow_exception(p.exception_);
    }
    return p.value_;
  }
  Awaiter operator co_await() & noexcept { return Awaiter{*this}; }
  R_Awaiter operator co_await() && noexcept {
    return R_Awaiter{std::move(*this)};
  }

private:
  std::coroutine_handle<promise_type> handle_;
};

template <> struct Task<void> {
  struct promise_type {
    std::exception_ptr exception_{};
    struct alignas(64) WaitingNode {
      std::coroutine_handle<> node_{nullptr};
      WaitingNode *next_{nullptr};
      WaitingNode() = default;
      WaitingNode(const WaitingNode &) = delete;
      WaitingNode &operator=(const WaitingNode &) = delete;
    };
    static WaitingNode *instance() {
      static WaitingNode dummy;
      return &dummy;
    }
    alignas(64) std::atomic<WaitingNode *> cache_{nullptr};
    alignas(64) std::atomic<std::size_t> ref_cnt{1};
    void add_ref() noexcept { ref_cnt.fetch_add(1, std::memory_order_acq_rel); }
    bool release_ref_and_is_last() {
      auto _last = ref_cnt.fetch_sub(1, std::memory_order_acq_rel);
      return _last == 1;
    }
    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_never initial_suspend() const noexcept { return {}; }
    struct Final_Awaiter {
      bool await_ready() const noexcept { return false; }
      std::coroutine_handle<>
      await_suspend(std::coroutine_handle<promise_type> h) noexcept {
        auto &handle = h.promise();
        WaitingNode *head =
            handle.cache_.exchange(instance(), std::memory_order_acq_rel);
        handle.cache_.notify_all();
        if (head && head != instance() && head->node_) {
          return head->node_;
        }
        return std::noop_coroutine();
      }
      void await_resume() noexcept {}
    };
    Final_Awaiter final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() { exception_ = std::current_exception(); }
  };
  explicit Task(std::coroutine_handle<promise_type> handle) : handle_(handle) {}
  ~Task() {
    if (this->handle_) {
      auto &p = this->handle_.promise();
      if (p.release_ref_and_is_last()) {
        if (p.cache_.load(std::memory_order_acquire) ==
            promise_type::instance()) {
          this->handle_.destroy();
        }
      }
    }
  }

  Task(const Task &) = delete;
  Task &operator=(const Task &) = delete;

  Task(Task &&obj) noexcept : handle_(std::exchange(obj.handle_, {})) {}
  Task &operator=(Task &&obj) noexcept {
    if (this != &obj) {
      if (this->handle_) {
        auto &p = this->handle_.promise();
        if (p.release_ref_and_is_last()) {
          if (p.cache_.load(std::memory_order_acquire) ==
              promise_type::instance()) {
            this->handle_.destroy();
          }
        }
      }
      this->handle_ = std::exchange(obj.handle_, {});
    }
    return *this;
  }
  struct Awaiter {
    Task &self;
    bool is_add_{false};
    explicit Awaiter(Task &task) : self(task) {}
    typename promise_type::WaitingNode Node{};
    bool await_ready() const noexcept {
      return self.handle_.promise().cache_.load(std::memory_order_acquire) ==
             promise_type::instance();
    }
    void await_resume() {
      auto &p = self.handle_.promise();
      auto *next = Node.next_;
      if (p.exception_) {
        if (next && next->node_)
          next->node_.resume();
        if (is_add_) {
          p.release_ref_and_is_last();
        }
        std::rethrow_exception(p.exception_);
      }
      if (next && next->node_)
        next->node_.resume();
      if (is_add_) {
        p.release_ref_and_is_last();
      }
    }
    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> awaiting) noexcept {
      auto &p = self.handle_.promise();

      auto &head = p.cache_;
      auto *head_node = head.load(std::memory_order_acquire);
      Node.node_ = awaiting;
      for (;;) {
        if (head_node == promise_type::instance())
          return awaiting;
        Node.next_ = head_node;
        if (head.compare_exchange_weak(head_node, &Node,
                                       std::memory_order_acq_rel,
                                       std::memory_order_acquire)) {
          p.add_ref();
          is_add_ = true;
          return std::noop_coroutine();
        }
      }
    }
  };
  struct R_Awaiter {
    std::coroutine_handle<promise_type> handle_;
    bool is_add_{false};
    explicit R_Awaiter(Task &&task)
        : handle_(std::exchange(task.handle_, {})) {}
    // 由于外部Task的所有权转移至此，外面Task的析构函数无法工作，相关资源释放转移至此处理
    ~R_Awaiter() {
      auto &p = handle_.promise();
      const bool is_last = p.release_ref_and_is_last();
      if (is_last && p.cache_.load(std::memory_order_acquire) ==
                         promise_type::instance()) {
        handle_.destroy();
      }
    }
    typename promise_type::WaitingNode Node{};
    bool await_ready() const noexcept {
      return handle_.promise().cache_.load(std::memory_order_acquire) ==
             promise_type::instance();
    }
    void await_resume() {
      auto &p = handle_.promise();
      auto *next = Node.next_;
      if (p.exception_) {
        if (next && next->node_)
          next->node_.resume();
        if (is_add_) {
          p.release_ref_and_is_last();
        }
        std::rethrow_exception(p.exception_);
      }
      if (next && next->node_)
        next->node_.resume();
      if (is_add_) {
        p.release_ref_and_is_last();
      }
      return;
    }
    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> awaiting) noexcept {
      auto &p = handle_.promise();
      auto &head = p.cache_;
      auto *head_node = head.load(std::memory_order_acquire);

      Node.node_ = awaiting;
      for (;;) {
        if (head_node == promise_type::instance())
          return awaiting;
        Node.next_ = head_node;
        if (head.compare_exchange_weak(head_node, &Node,
                                       std::memory_order_acq_rel,
                                       std::memory_order_acquire)) {
          p.add_ref();
          is_add_ = true;
          return std::noop_coroutine();
        }
      }
    }
  };
  bool is_ready() const noexcept {
    if (!handle_)
      return true;
    auto &p = handle_.promise();
    return p.cache_.load(std::memory_order_acquire) == promise_type::instance();
  }
  void get() {
    if (!handle_)
      throw std::logic_error("Task used after move");
    auto &p = handle_.promise();
    if (p.cache_.load(std::memory_order_acquire) != promise_type::instance()) {
      auto expected = p.cache_.load(std::memory_order_acquire);
      while (expected != promise_type::instance()) {
        p.cache_.wait(expected, std::memory_order_acquire);
        expected = p.cache_.load(std::memory_order_acquire);
      }
    }
    if (p.exception_) {
      std::rethrow_exception(p.exception_);
    }
    return;
  }

  void get_blocking() {
    if (!handle_)
      throw std::logic_error("Task used after move");
    auto &p = handle_.promise();

    if (p.cache_.load(std::memory_order_acquire) == promise_type::instance()) {
      if (p.exception_) {
        std::rethrow_exception(p.exception_);
      }
      return;
    }
    auto expected = p.cache_.load(std::memory_order_acquire);
    while (expected != promise_type::instance()) {
      p.cache_.wait(expected, std::memory_order_acquire);
      expected = p.cache_.load(std::memory_order_acquire);
    }
    if (p.exception_) {
      std::rethrow_exception(p.exception_);
    }
    return;
  }
  Awaiter operator co_await() & noexcept { return Awaiter{*this}; }
  R_Awaiter operator co_await() && noexcept {
    return R_Awaiter{std::move(*this)};
  }

private:
  std::coroutine_handle<promise_type> handle_;
};
