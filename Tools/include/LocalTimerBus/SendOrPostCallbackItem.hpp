#pragma once

#include <exception>
#include <functional>
#include <future>
#include <utility>

class SendOrPostCallbackItem {
public:
  using CallBack = std::function<void()>;
  SendOrPostCallbackItem(CallBack callback, bool catch_exception)
      : callback_(std::move(callback)), catch_exception_(catch_exception) {}
  void Run() {
    if (catch_exception_) {
      try {
        if (callback_) {
          callback_();
        }
      } catch (...) {
        exception_ = std::current_exception();
      }
      promise_.set_value();
    } else {
      if (callback_) {
        callback_();
      }
    }
  }
  void Wait() {
    future_.wait();
    if (exception_) {
      promise_.set_exception(exception_);
    }
  }
  std::exception_ptr GetException() const noexcept { return exception_; }

private:
  CallBack callback_;
  bool catch_exception_;
  std::exception_ptr exception_;
  std::promise<void> promise_;
  std::future<void> future_{promise_.get_future()};
};
