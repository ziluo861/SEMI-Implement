#pragma once
#include <DataType/FormatCode.hpp>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

class SECSItemBase {
public:
  virtual ~SECSItemBase() = default;
  virtual FormatCode TryGetFormat() = 0;
  virtual bool TryParseContent(std::string_view) = 0;
  virtual std::string TryDeparseContent(int level = 0) = 0;
  virtual std::optional<std::vector<std::uint8_t>> TrySerialize() = 0;
  virtual bool TrySerialize(std::vector<std::uint8_t> &) = 0;
  virtual bool TryDeserialize(std::span<std::uint8_t> &bytes, int length) = 0;
  virtual std::size_t Size() noexcept {
    return std::numeric_limits<std::size_t>::max();
  }

protected:
  SECSItemBase() = default;
};
