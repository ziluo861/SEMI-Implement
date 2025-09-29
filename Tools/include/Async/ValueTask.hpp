#pragma once
#include "Async/Task.hpp"
#include <variant>
#include <optional>
#include <concepts>

template <typename T> class ValueTask {
private:
  std::variant<T, Task<T>> storage_;
  bool has_inline_value() const noexcept {
    return std::holds_alternative<T>(storage_);
  }

public:
  explicit ValueTask(T val) : storage_(std::move(val)) {}
  explicit ValueTask(Task<T> task) : storage_(std::move(task)) {}

  template <typename U>
    requires std::convertible_to<U, T>
  ValueTask(U &&val) : storage_(T(std::forward<U>(val))) {}

  bool is_ready() const noexcept {
    if (has_inline_value())
      return true;
    auto &task = std::get<Task<T>>(storage_);
    return task.is_ready();
  }

  T get() {
    if (auto *value = std::get_if<T>(&storage_)) {
      return *value;
    }
    return std::get<Task<T>>(storage_).get();
  }

  T get_blocking() {
    if (auto *value = std::get_if<T>(&storage_)) {
      return *value;
    }
    return std::get<Task<T>>(storage_).get_blocking();
  }

  struct Awaiter {
    ValueTask &self;
    std::optional<typename Task<T>::Awaiter> inner_task_{};
    void ensure_task_awaiter() {
      if (!inner_task_) {
        auto &_task = std::get<Task<T>>(self.storage_);
        inner_task_.emplace(_task);
      }
    }
    bool await_ready() noexcept {
      if (self.has_inline_value())
        return true;
      ensure_task_awaiter();
      return inner_task_->await_ready();
    }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting) {
      ensure_task_awaiter();
      return inner_task_->await_suspend(awaiting);
    }
    T await_resume() {
      if (auto *value = std::get_if<T>(&self.storage_)) {
        return *value;
      }
      ensure_task_awaiter();
      return inner_task_->await_resume();
    }
  };

  struct R_Awaiter {
    std::variant<T, Task<T>> storage_;
    std::optional<typename Task<T>::R_Awaiter> inner_task_{};
    void ensure_task_awaiter() {
      if (!inner_task_) {
        auto &_task = std::get<Task<T>>(storage_);
        inner_task_.emplace(std::move(_task));
      }
    }
    bool await_ready() noexcept {
      if (std::holds_alternative<T>(storage_))
        return true;
      ensure_task_awaiter();
      return inner_task_->await_ready();
    }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting) {
      ensure_task_awaiter();
      return inner_task_->await_suspend(awaiting);
    }
    T await_resume() {
      if (auto *value = std::get_if<T>(&storage_)) {
        return *value;
      }
      ensure_task_awaiter();
      return inner_task_->await_resume();
    }
  };

  Awaiter operator co_await() & noexcept { return Awaiter{*this}; }

  R_Awaiter operator co_await() && noexcept {
    return R_Awaiter{std::move(storage_)};
  }

  Awaiter operator co_await() const & = delete;
  R_Awaiter operator co_await() const && = delete;
};

template <> class ValueTask<void> {
private:
  std::variant<std::monostate, Task<void>> storage_;
  bool has_inline_value() const noexcept {
    return std::holds_alternative<std::monostate>(storage_);
  }

public:
  explicit ValueTask() : storage_(std::monostate{}) {}
  explicit ValueTask(Task<void> task) : storage_(std::move(task)) {}

  bool is_ready() const noexcept {
    if (has_inline_value())
      return true;
    auto &task = std::get<Task<void>>(storage_);
    return task.is_ready();
  }

  void get() {
    if (has_inline_value()) {
      return;
    }
    std::get<Task<void>>(storage_).get();
    return;
  }

  void get_blocking() {
    if (has_inline_value()) {
      return;
    }
    std::get<Task<void>>(storage_).get_blocking();
    return;
  }

  struct Awaiter {
    ValueTask &self;
    std::optional<typename Task<void>::Awaiter> inner_task_{};
    void ensure_task_awaiter() {
      if (!inner_task_) {
        auto &_task = std::get<Task<void>>(self.storage_);
        inner_task_.emplace(_task);
      }
    }
    bool await_ready() noexcept {
      if (self.has_inline_value())
        return true;
      ensure_task_awaiter();
      return inner_task_->await_ready();
    }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting) {
      ensure_task_awaiter();
      return inner_task_->await_suspend(awaiting);
    }
    void await_resume() {
      if (self.has_inline_value()) {
        return;
      }
      ensure_task_awaiter();
      inner_task_->await_resume();
      return;
    }
  };

  struct R_Awaiter {
    std::variant<std::monostate, Task<void>> storage_;
    std::optional<typename Task<void>::R_Awaiter> inner_task_{};
    void ensure_task_awaiter() {
      if (!inner_task_) {
        auto &_task = std::get<Task<void>>(storage_);
        inner_task_.emplace(std::move(_task));
      }
    }
    bool await_ready() noexcept {
      if (std::holds_alternative<std::monostate>(storage_))
        return true;
      ensure_task_awaiter();
      return inner_task_->await_ready();
    }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting) {
      ensure_task_awaiter();
      return inner_task_->await_suspend(awaiting);
    }
    void await_resume() {
      if (std::holds_alternative<std::monostate>(storage_))
        return;
      ensure_task_awaiter();
      return inner_task_->await_resume();
    }
  };

  Awaiter operator co_await() & noexcept { return Awaiter{*this}; }

  R_Awaiter operator co_await() && noexcept {
    return R_Awaiter{std::move(storage_)};
  }

  Awaiter operator co_await() const & = delete;
  R_Awaiter operator co_await() const && = delete;
};

template <typename T> ValueTask<T> completed_value_task(T value) {
  return ValueTask<T>(std::move(value));
}

inline ValueTask<void> completed_value_task() { return ValueTask<void>(); }

template <typename T> ValueTask<T> from_task(Task<T> task) {
  return ValueTask<T>(std::move(task));
}