#pragma once
#include <cstdint>
#include <sys/stat.h>
class SECSHead {
public:
  using valuetype = std::uint8_t;

private:
  valuetype stream_byte_;
  valuetype function_byte_;

public:
  static constexpr valuetype wbit_flag_ = 0x80;
  static constexpr valuetype stream_filter_ = 0x7F;
  valuetype StreamByte() const noexcept;
  valuetype FunctionByte() const noexcept;

  valuetype Stream() const noexcept;
  valuetype Function() const noexcept;

  bool NeedReply() const noexcept;

  valuetype ReplyStreamByte() const noexcept;
  valuetype ReplyFunctionByte() const noexcept;

  explicit SECSHead(valuetype stream, valuetype function, bool needReply);
  explicit SECSHead(valuetype streamByte, valuetype functionByte);

  static bool CheckRequestReply(valuetype, valuetype, valuetype,
                                valuetype) noexcept;
};
