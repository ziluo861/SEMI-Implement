#pragma once
#include <SECS/SECSItem.hpp>
#include <bit>
#include <cstring>

class FloatItem : public SECSItem<FloatItem> {
private:
  using type = float;
  std::vector<type> values;
  static constexpr std::uint8_t LogOfElemBytesCount =
      std::bit_width(sizeof(type)) - 1;
  static constexpr std::uint8_t ElemBytesCount = sizeof(type);
  static constexpr std::uint8_t SizeFilter = sizeof(type) - 1;

public:
  FormatCode GetFormat() const noexcept { return FormatCode::FloatFormatCode; }
  std::size_t Size() noexcept { return values.size() << LogOfElemBytesCount; }
  bool ParseContent(std::string_view _context) noexcept {
    auto subStrings = StringUtils::SplitAndRemoveEmpty(_context);
    values.clear();
    values.reserve(subStrings.size());
    for (const auto &sub : subStrings) {
      type _value;
      if (!StringUtils::ParseValue<type>(sub, _value)) {
        return false;
      }
      values.emplace_back(_value);
    }
    return true;
  }
  std::string DeparseContent([[maybe_unused]] int level) const noexcept {
    std::ostringstream builder;
    for (const auto &_value : values) {
      builder << ' ' << StringUtils::DeparseValue(_value);
    }
    return builder.str();
  }
  bool Serialize(std::vector<std::uint8_t> &bytes) const {
    try {
      size_t oldSize = bytes.size();
      bytes.resize(oldSize + values.size() * sizeof(type));
      for (size_t i = 0; i < values.size(); ++i) {
        std::uint32_t u;
        std::memcpy(&u, &values[i], sizeof(u));

        size_t offset = oldSize + i * sizeof(type);
        bytes[offset + 0] = static_cast<std::uint8_t>(u >> 24);
        bytes[offset + 1] = static_cast<std::uint8_t>(u >> 16);
        bytes[offset + 2] = static_cast<std::uint8_t>(u >> 8);
        bytes[offset + 3] = static_cast<std::uint8_t>(u >> 0);
      }
    } catch (...) {
      return false;
    }
    return true;
  }
  bool Deserialize(std::span<std::uint8_t> &bytes, int length) {
    values.clear();
    if (length < 0 || length > static_cast<int>(bytes.size()) ||
        (length & SizeFilter) != 0) {
      return false;
    }
    values.reserve(length / ElemBytesCount);
    for (int i = 0; i < length; i += ElemBytesCount) {
      try {
        auto _byte = bytes.subspan(i, ElemBytesCount);
        std::uint32_t _value = (static_cast<std::uint64_t>(_byte[0]) << 24) |
                               (static_cast<std::uint64_t>(_byte[1]) << 16) |
                               (static_cast<std::uint64_t>(_byte[2]) << 8) |
                               _byte[3];
        type _tmp = 0;
        std::memcpy(&_tmp, &_value, sizeof(type));
        values.emplace_back(_tmp);
      } catch (...) {
        return false;
      }
    }
    bytes = bytes.subspan(length);
    return true;
  }
};
