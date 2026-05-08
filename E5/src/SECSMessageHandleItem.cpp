#include "SECS/SentinelSECSItem.hpp"
#include <SECSMessageHandleItem.hpp>
#include <memory>

SECSItemBase *SECSMessageHandleItem::RejectItem() noexcept {
  static SentinelSECSItem instance;
  return &instance;
}

const std::vector<std::uint8_t> &
SECSMessageHandleItem::RemoteHeadBytes() const noexcept {
  return remote_head_bytes_;
}

std::shared_ptr<SECSHead> SECSMessageHandleItem::ReplyHead() noexcept {
  return std::make_shared<SECSHead>(
      request_head_->Stream(),
      static_cast<std::uint8_t>(
          reply_item_.get() == RejectItem() ? 0u : request_head_->Function() + 1u),
      false);
}

 std::shared_ptr<SECSHead> SECSMessageHandleItem::RequestHead() const noexcept {
  return request_head_;
}

 std::shared_ptr<SECSItemBase> SECSMessageHandleItem::RequestItem() const noexcept {
  return request_item_;
}

std::shared_ptr<SECSItemBase> SECSMessageHandleItem::ReplyItem() const noexcept {
  return reply_item_;
}

void SECSMessageHandleItem::Set_ReplyItem(std::shared_ptr<SECSItemBase> item) noexcept {
  if (reply_item_ == nullptr) {
    reply_item_ = item;
  }
}

SECSMessageHandleItem::SECSMessageHandleItem(std::shared_ptr<SECSHead> head, std::shared_ptr<SECSItemBase> item)
    : request_head_(head), request_item_(item), reply_item_(nullptr) {}
