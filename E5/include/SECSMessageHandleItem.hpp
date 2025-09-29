#pragma once

#include "SECS/SECSBase.hpp"
#include "SECSHead/SECSHead.hpp"
#include <cstdint>
#include <vector>

class SECSMessageHandleItem {
private:
  std::vector<std::uint8_t> remote_head_bytes_;
  SECSHead *request_head_ = nullptr;
  SECSItemBase *request_item_ = nullptr;
  SECSItemBase *reply_item_ = nullptr;

public:
  static SECSItemBase *RejectItem() noexcept;
  const SECSHead *RequestHead() const noexcept;
  const SECSItemBase *RequestItem() const noexcept;
  const std::vector<std::uint8_t> &RemoteHeadBytes() const noexcept;
  SECSHead ReplyHead() noexcept;
  const SECSItemBase *ReplyItem() const noexcept;
  void Set_ReplyItem(SECSItemBase *) noexcept;

  explicit SECSMessageHandleItem(SECSHead *, SECSItemBase *);
};
