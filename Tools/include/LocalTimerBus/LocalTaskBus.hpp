#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <stop_token>
#include <thread>

class LocalTaskBus {
private:
  std::queue<std::function<void()>> que_;
  std::mutex mutex_;
  std::condition_variable cond_;
  std::atomic<bool> stopping_{false};
  std::jthread threads_;
  void start() {
    threads_ =
        std::jthread([this](std::stop_token token) { this->work(token); });
  }
  void work(std::stop_token token) {
    for (;;) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this, &token]() {
          return !que_.empty() || token.stop_requested();
        });
        if (token.stop_requested() && que_.empty()) {
          break;
        }
        task = std::move(que_.front());
        que_.pop();
      }
      task();
    }
  }

public:
  explicit LocalTaskBus() { start(); }

  ~LocalTaskBus() {
    stopping_.store(true, std::memory_order_release);
    threads_.request_stop();
    cond_.notify_all();
  }

  LocalTaskBus(const LocalTaskBus &) = delete;
  LocalTaskBus &operator=(const LocalTaskBus &) = delete;
  LocalTaskBus(LocalTaskBus &&) = delete;
  LocalTaskBus &operator=(LocalTaskBus &&) = delete;

  template <typename Callable, typename... ARGS>
  auto AddTask(Callable &&callable, ARGS &&...args) {
    if (stopping_.load(std::memory_order_acquire)) {
      throw std::runtime_error("ThreadPool is stopping");
    }
    using value_type =
        std::invoke_result_t<std::decay_t<Callable>, std::decay_t<ARGS>...>;
    auto task = std::make_unique<std::packaged_task<value_type()>>(
        [_callable = std::forward<Callable>(callable),
         ... _args = std::forward<ARGS>(args)]() mutable {
          return std::invoke(std::move(_callable), std::move(_args)...);
        });
    auto future = task->get_future();
    {
      std::lock_guard lock(mutex_);
      if (stopping_.load(std::memory_order_acquire)) {
        throw std::runtime_error("ThreadPool is stopping");
      }
      que_.emplace([task]() { (*task)(); });
    }
    cond_.notify_one();
    return future;
  }
};
