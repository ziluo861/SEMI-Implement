#pragma once
#include <concepts>
#include <functional>

template <typename T>
concept StateIndex = std::equality_comparable<T> && requires(T t) {
  std::hash<T>{}(t);
} && !std::is_pointer_v<T> && std::is_copy_constructible_v<T>;
