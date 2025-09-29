#pragma once
#include <DataType/FormatCode.hpp>
#include <SECS/SECSBase.hpp>
#include <SECS/SECSFactory.hpp>
#include <Utils/StringUtils.hpp>
#include <memory>
#include <optional>
class SECSParser {
private:
  static constexpr char RangeStartMark = '<';
  static constexpr char RangeEndMark = '>';
  static constexpr std::uint8_t LengthBytesCountFilter = 0x03;
  static constexpr std::uint8_t FormatCodeFilter = 0xFC;
  using ParseResult = std::optional<std::unique_ptr<SECSItemBase>>;
  static bool GetItemWithoutContent(std::string_view raw, std::string &context,
                                    std::string &base) noexcept {
    std::smatch base_match;
    std::string str{raw};
    if (std::regex_search(str, base_match, CodeNameExtension::ParseReg)) {
      if (base_match.size() >= 1) {
        context = base_match.suffix().str();
        base = base_match[0].str();
        return true;
      }
    }
    return false;
    ;
  }

public:
  static ParseResult TryParseContent(std::string_view _Raw) {
    auto _Trim = StringUtils::trim(_Raw);
    auto _size = _Trim.size();
    if (_size < 2 || _Trim[0] != RangeStartMark ||
        _Trim[_size - 1] != RangeEndMark) {
      return std::nullopt;
    }
    auto _context = _Trim.substr(1, _size - 2);
    std::string values, matched;
    if (!GetItemWithoutContent(_context, values, matched)) {
      return std::nullopt;
    }
    auto _formatCode = CodeNameExtension::GetNameCode(matched);
    auto _item = SECSFactory::createItem(_formatCode);
    if (_item && _item->TryParseContent(values)) {
      return _item;
    }
    return std::nullopt;
  }
  static bool TryDeserialize(std::span<std::uint8_t> &bytes,
                             ParseResult &_item) {
    _item.reset();
    if (bytes.empty()) {
      return true;
    }
    auto bytesFormat = bytes[0];
    int lengthBytesLength =
        static_cast<int>(bytesFormat & LengthBytesCountFilter);
    if (lengthBytesLength == 0 ||
        lengthBytesLength >= static_cast<int>(bytes.size())) {
      return false;
    }
    _item = SECSFactory::createItem(
        static_cast<FormatCode>(bytesFormat & FormatCodeFilter));
    if (!_item.has_value()) {
      return false;
    }
    std::uint32_t length = 0;
    for (int i = 1; i <= lengthBytesLength; ++i) {
      length = (length << 8) | bytes[i];
    }
    bytes = bytes.subspan(lengthBytesLength + 1);
    return _item.value()->TryDeserialize(bytes, static_cast<int>(length));
  }
};
