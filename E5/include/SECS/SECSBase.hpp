#pragma once
#include <FormatCode.hpp>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <vector>
#include "SECSConverter.hpp"

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
    template <typename T>
    static bool ConvertTo(SECSItemBase& item, T& value)
    {
        using U = std::remove_cvref_t<T>;
        return SECSConverter<U>::to(item, value);
    }

    template <typename T>
    static std::unique_ptr<SECSItemBase> ConvertFrom(T&& value)
    {
        using U = std::remove_cvref_t<T>;
        return SECSConverter<U>::from(std::forward<T>(value));
    }

protected:
  SECSItemBase() = default;
};
