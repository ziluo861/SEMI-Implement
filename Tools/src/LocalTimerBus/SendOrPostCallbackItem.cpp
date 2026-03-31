#include "LocalTimerBus/SendOrPostCallbackItem.hpp"
#include <utility>

SendOrPostCallbackItem::SendOrPostCallbackItem(CallBack callback,
                                               bool catch_exception)
    : callback_(std::move(callback)), catch_exception_(catch_exception) {}

void SendOrPostCallbackItem::Run() {
  if (catch_exception_) {
    try {
      if (callback_) {
        callback_();
      }
    } catch (...) {
      exception_ = std::current_exception();
    }
  } else {
    if (callback_) {
      callback_();
    }
  }
  promise_.set_value();
}

void SendOrPostCallbackItem::Wait() {
  auto dummy_future = promise_.get_future();
  dummy_future.wait();
  if (exception_) {
    promise_.set_exception(exception_);
  }
}
