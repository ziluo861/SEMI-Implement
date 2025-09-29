#include "SECS/SentinelSECSItem.hpp"
#include <SECSMessageHandleItem.hpp>

SECSItemBase *SECSMessageHandleItem::RejectItem() noexcept {
  static SentinelSECSItem instance;
  return &instance;
}

const std::vector<std::uint8_t> &
SECSMessageHandleItem::RemoteHeadBytes() const noexcept {
  return remote_head_bytes_;
}

SECSHead SECSMessageHandleItem::ReplyHead() noexcept {
  return SECSHead(
      request_head_->Stream(),
      static_cast<std::uint8_t>(
          reply_item_ == RejectItem() ? 0u : request_head_->Function() + 1u),
      false);
}

const SECSHead *SECSMessageHandleItem::RequestHead() const noexcept {
  return request_head_;
}

const SECSItemBase *SECSMessageHandleItem::RequestItem() const noexcept {
  return request_item_;
}

const SECSItemBase *SECSMessageHandleItem::ReplyItem() const noexcept {
  return reply_item_;
}

void SECSMessageHandleItem::Set_ReplyItem(SECSItemBase *item) noexcept {
  if (reply_item_ == nullptr) {
    reply_item_ = item;
  }
}

SECSMessageHandleItem::SECSMessageHandleItem(SECSHead *head, SECSItemBase *item)
    : request_head_(head), request_item_(item), reply_item_(nullptr) {}
