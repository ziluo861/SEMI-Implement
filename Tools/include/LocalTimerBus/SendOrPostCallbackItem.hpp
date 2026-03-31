#pragma once

#include <exception>
#include <functional>
#include <future>

class SendOrPostCallbackItem {
public:
  using CallBack = std::function<void()>;
  SendOrPostCallbackItem(CallBack callback, bool catch_exception);
  void Run();
  void Wait();
  inline std::exception_ptr GetException() const noexcept { return exception_; }

private:
  CallBack callback_;
  bool catch_exception_;
  std::exception_ptr exception_;
  std::promise<void> promise_;
};
