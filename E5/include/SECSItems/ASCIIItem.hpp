#pragma once
#include <SECSItem.hpp>
#include <StringUtils.hpp>
#include <string>
#include <string_view>

class ASCIIItem : public SECSItem<ASCIIItem> {
private:
  std::string value_;

public:
  FormatCode GetFormat() const noexcept { return FormatCode::ASCIIFormatCode; }
  std::string Value() const {
    return value_;
  }
  bool ParseContent(std::string_view _context) {
    value_ = StringUtils::trim(_context);
    return true;
  }
  std::string DeparseContent([[maybe_unused]] int level) const {
    return ' ' + value_;
  }
  std::size_t Size() noexcept { return value_.size(); }
  bool Serialize(std::vector<std::uint8_t> &bytes) const {
    if (value_.empty()) {
      return true;
    }
    bytes.reserve(bytes.size() + value_.size());
    bytes.insert(bytes.end(), value_.begin(), value_.end());
    return true;
  }
  bool Deserialize(std::span<std::uint8_t> &bytes, int length) {
    if (length < 0 || length > static_cast<int>(bytes.size())) {
      return false;
    }
    value_.reserve(length);
    value_.assign(reinterpret_cast<const char *>(bytes.data()), length);
    bytes = bytes.subspan(length);
    return true;
  }
};
