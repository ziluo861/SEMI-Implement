#pragma once
#include <SECS/SECSBase.hpp>
#include <SECS/SECSItem.hpp>
#include <SECS/SECSParser.hpp>
#include <memory>
#include <vector>

class ListItem : public SECSItem<ListItem> {
private:
  std::vector<std::unique_ptr<SECSItemBase>> valueItems;

public:
  FormatCode GetFormat() const noexcept { return FormatCode::ListFormatCode; }
  std::size_t Size() noexcept { return valueItems.size(); }
  bool ParseContent(std::string_view _context) {
    valueItems.clear();
    if (_context.empty()) {
      return true;
    }
    const int _len = static_cast<int>(_context.size());
    int count = 0, lastIndex = -1;
    for (int i = 0; i < _len; i++) {
      if (_context[i] == RangeStartMark) {
        count += 1;
      } else if (_context[i] == RangeEndMark) {
        count -= 1;
        if (count == 0) {
          auto _item = SECSParser::TryParseContent(
              _context.substr(lastIndex + 1, i - lastIndex));
          if (!_item.has_value()) {
            return false;
          }
          valueItems.emplace_back(std::move(_item.value()));
          lastIndex = i;
        }
      } else if (count == 0 &&
                 !std::isspace(static_cast<unsigned char>(_context[i]))) {
        return false;
      }
    }
    return count == 0;
  }
  std::string DeparseContent(int level) const {
    std::ostringstream builder;
    for (const auto &subItem : valueItems) {
      builder << std::endl << subItem->TryDeparseContent(level + 1);
    }
    if (!valueItems.empty()) {
      builder << std::endl << GetPrefix(level);
    }
    return builder.str();
  }
  bool Serialize(std::vector<std::uint8_t> &bytes) const {
    for (const auto &_subItem : valueItems) {
      if (!_subItem->TrySerialize(bytes)) {
        return false;
      }
    }
    return true;
  }
  bool Deserialize(std::span<std::uint8_t> &bytes, int length) {
    valueItems.clear();
    valueItems.reserve(length);
    for (int i = 0; i < length; i++) {
      std::optional<std::unique_ptr<SECSItemBase>> _subItem;
      auto _result = SECSParser::TryDeserialize(bytes, _subItem);
      if (!_result || !_subItem.has_value()) {
        return false;
      }
      valueItems.emplace_back(std::move(_subItem.value()));
    }
    return true;
  }
};
