#pragma once

#include "SECS/SECSBase.hpp"
#include "SECSHead/SECSHead.hpp"
#include <cstdint>
#include <memory>
#include <vector>

class SECSMessageHandleItem {
private:
  std::vector<std::uint8_t> remote_head_bytes_;
  std::shared_ptr<SECSHead> request_head_ = nullptr;
  std::shared_ptr<SECSItemBase> request_item_ = nullptr;
  std::shared_ptr<SECSItemBase> reply_item_ = nullptr;

public:
  SECSItemBase * RejectItem() noexcept;
  std::shared_ptr<SECSHead> RequestHead() const noexcept;
  std::shared_ptr<SECSItemBase> RequestItem() const noexcept;
  const std::vector<std::uint8_t> &RemoteHeadBytes() const noexcept;
  std::shared_ptr<SECSHead> ReplyHead() noexcept;
  std::shared_ptr<SECSItemBase> ReplyItem() const noexcept;
  void Set_ReplyItem(std::shared_ptr<SECSItemBase>) noexcept;

  explicit SECSMessageHandleItem(std::shared_ptr<SECSHead>, std::shared_ptr<SECSItemBase>);
};
