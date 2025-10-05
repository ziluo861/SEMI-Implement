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
#include <vector>

class ThreadPool {
private:
  std::queue<std::function<void()>> que_;
  unsigned int nums_;
  std::mutex mutex_;
  std::condition_variable cond_;
  std::atomic<bool> stopping_{false};
  std::vector<std::jthread> threads_;
  void start() {
    for (unsigned int i = 0; i < nums_; ++i) {
      threads_.emplace_back(
          [this](std::stop_token token) { this->work(token); });
    }
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
  explicit ThreadPool(unsigned int nums) {
    unsigned int hwc = std::max(1u, std::thread::hardware_concurrency());
    unsigned int min_threads = 1u; // 或 2u 按你需求
    nums_ = std::clamp(nums, min_threads, hwc);
    threads_.reserve(nums_);
    start();
  }

  ~ThreadPool() {
    stopping_.store(true, std::memory_order_release);
    for (auto &t : threads_) {
      t.request_stop();
    }
    cond_.notify_all();
  }

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool &operator=(ThreadPool &&) = delete;

  template <typename Callable, typename... ARGS>
  auto AddTask(Callable &&callable, ARGS &&...args) {
    if (stopping_.load(std::memory_order_acquire)) {
      throw std::runtime_error("ThreadPool is stopping");
    }
    using value_type =
        std::invoke_result_t<std::decay_t<Callable>, std::decay_t<ARGS>...>;
    auto task = std::make_shared<std::packaged_task<value_type()>>(
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