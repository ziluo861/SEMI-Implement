
#include "SECSHead/SECSHead.hpp"

SECSHead::valuetype SECSHead::StreamByte() const noexcept {
  return stream_byte_;
}

SECSHead::valuetype SECSHead::FunctionByte() const noexcept {
  return function_byte_;
}

SECSHead::valuetype SECSHead::Stream() const noexcept {
  return static_cast<valuetype>(stream_byte_ & stream_filter_);
}

SECSHead::valuetype SECSHead::Function() const noexcept {
  return function_byte_;
}

bool SECSHead::NeedReply() const noexcept {
  return (stream_byte_ & wbit_flag_) != 0u;
}

SECSHead::valuetype SECSHead::ReplyStreamByte() const noexcept {
  return static_cast<valuetype>(stream_byte_ & stream_filter_);
}

SECSHead::valuetype SECSHead::ReplyFunctionByte() const noexcept {
  return static_cast<valuetype>(function_byte_ + 1u);
}

SECSHead::SECSHead(valuetype stream, valuetype function, bool needReply) {
  if ((function & 1u) == 0u) {
    needReply = false;
  }
  if (needReply) {
    stream_byte_ = static_cast<valuetype>(stream | wbit_flag_);
  } else {
    stream_byte_ = static_cast<valuetype>(stream & stream_filter_);
  }
  function_byte_ = function;
}

SECSHead::SECSHead(valuetype streamByte, valuetype functionByte) {
  stream_byte_ = static_cast<valuetype>(
      ((functionByte & 1u) == 0u) ? (streamByte & stream_filter_) : streamByte);
  function_byte_ = functionByte;
}

bool SECSHead::CheckRequestReply(valuetype reqStreamByte,
                                 valuetype reqFunctionByte,
                                 valuetype resStreamByte,
                                 valuetype resFunctionByte) noexcept {
  return ((reqStreamByte ^ resStreamByte) == wbit_flag_) &&
         ((reqStreamByte & wbit_flag_) != 0u) &&
         (static_cast<valuetype>(reqFunctionByte + 1u) == resFunctionByte) &&
         ((reqFunctionByte & 1u) != 0u);
}
