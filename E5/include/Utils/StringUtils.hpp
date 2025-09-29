#pragma once
#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

class StringUtils {
public:
  static inline constexpr std::array<bool, 256> CreateSplitterLookup() {
    std::array<bool, 256> lookUP{};
    constexpr const char *ValuesSpliters = " \a\b\f\t\v\n\r";
    for (const char *_char = ValuesSpliters; *_char; _char++) {
      lookUP[static_cast<unsigned char>(*_char)] = true;
    }

    return lookUP;
  }
  static inline std::array<bool, 256> SplitterLookup = CreateSplitterLookup();
  [[nodiscard]]
  static inline bool IsSplitter(char c) noexcept {
    return SplitterLookup[static_cast<unsigned char>(c)];
  }
  [[nodiscard]]
  static inline std::vector<std::string_view>
  SplitAndRemoveEmpty(std::string_view str) noexcept {
    if (str.empty()) {
      return {};
    }
    std::vector<std::string_view> result;
    std::size_t _count = 0;
    for (const auto &c : str) {
      if (IsSplitter(c)) {
        ++_count;
      }
    }
    result.reserve(_count + 1);
    size_t start = 0, _size = str.length();
    for (size_t i = 0; i < _size; i++) {
      if (IsSplitter(str[i])) {
        if (start < i) {
          result.emplace_back(str.substr(start, i - start));
        }
        start = i + 1;
      }
    }
    if (start < _size) {
      result.emplace_back(str.substr(start));
    }
    return result;
  }
  [[nodiscard]]
  static inline std::string_view trim(std::string_view str) noexcept {
    // std::string_view trimSpliters = " \t\n\r\f\v";
    // size_t start = str.find_first_not_of(trimSpliters);
    // if (start == std::string_view::npos) {
    //     return {};
    // }
    // size_t end = str.find_last_not_of(trimSpliters);
    // return str.substr(start, end - start + 1);
    if (str.empty()) {
      return {};
    }
    const char *begin = str.data();
    const char *end = begin + str.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(*begin))) {
      ++begin;
    }
    if (begin == end) {
      return {};
    }
    --end;
    while (end > begin && std::isspace(static_cast<unsigned char>(*end))) {
      --end;
    }
    return std::string_view(begin, end - begin + 1);
  }
  [[nodiscard]]
  static inline bool IsNullOrWhiteSpace(std::string_view str) noexcept {
    // return str.empty() || std::all_of(str.begin(), str.end(), [](unsigned
    // char _char) {
    //     return std::isspace(_char);
    // });
    if (str.empty()) {
      return true;
    }
    const char *begin = str.data();
    const char *end = begin + str.size();
    while (begin < end) {
      if (!std::isspace(static_cast<unsigned char>(*begin))) {
        return false;
      }
      ++begin;
    }
    return true;
  }
  template <typename T>
  [[nodiscard]]
  static bool ParseValue(std::string_view str, T &result) noexcept {
    if (str.empty()) {
      return false;
    }
    if constexpr (std::is_floating_point_v<T>) {
      char *end;
      errno = 0;
      std::string raw{str};
      if constexpr (std::is_same_v<T, float>) {
        result = std::strtof(raw.c_str(), &end);
      } else if (std::is_same_v<T, double>) {
        result = std::strtod(raw.c_str(), &end);
      } else {
        return false;
      }
      return (end == raw.c_str() + raw.size()) && errno != EAGAIN;
    } else {
      auto _first = str.data();
      auto _last = _first + str.size();
      auto [ptr, err] = std::from_chars(_first, _last, result);
      return err == std::errc{} && ptr == _last;
    }
    return false;
  }
  template <typename T>
  [[nodiscard]]
  static std::string DeparseValue(T value) {
    if constexpr (std::is_same_v<T, bool>) {
      return value ? "true" : "false";
    } else if constexpr (std::is_integral_v<T>) {
      return DeparseIntegral(value);
    } else if constexpr (std::is_floating_point_v<T>) {
      return DeparseFloating(value);
    } else {
      static_assert(std::is_arithmetic_v<T>,
                    "Unsupported type for DeparseValue");
    }
  }

private:
  template <typename T>
  [[nodiscard]]
  static std::string DeparseIntegral(T value) {
    static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>,
                  "Only supports integral types");
    constexpr std::size_t _guess = []() {
      if constexpr (std::is_signed_v<T>) {
        return static_cast<std::size_t>(std::numeric_limits<T>::digits10) + 3;
      }
      return static_cast<std::size_t>(std::numeric_limits<T>::digits10) + 2;
    }();
    std::array<char, _guess> buffer{};
    std::to_chars_result result =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (result.ec == std::errc{}) {
      return std::string(buffer.data(), result.ptr - buffer.data());
    }
    return std::to_string(value);
  }

  template <typename T>
  [[nodiscard]]
  static std::string DeparseFloating(T value, int Precision = 6) {
    static_assert(std::is_floating_point_v<T>, "Only for floating points");

    std::array<char, 128> buffer{};
    std::to_chars_result result =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value,
                      std::chars_format::fixed, Precision);
    if (result.ec == std::errc{}) {
      return std::string(buffer.data(), result.ptr - buffer.data());
    }
    return std::to_string(value);
  }
};
