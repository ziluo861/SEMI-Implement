#pragma once
#include "VarRef.hpp"
#include <concepts>

template <typename TValue>
  requires EqualityComparable<TValue> && std::copy_constructible<TValue>
class SourceVarRef : public VarRef<TValue> {
private:
  using BaseType = VarRef<TValue>;
  using ValueType = typename VarRef<TValue>::ValueType;
  using ComparerType = typename VarRef<TValue>::ComparerType;

public:
  explicit SourceVarRef(const ValueType &default_value,
                        ComparerType comparer = nullptr)
      : BaseType(default_value, std::move(comparer)) {}

  void set_value(ValueType value) { BaseType::set_value(std::move(value)); }

  const ValueType &value() const noexcept { return BaseType::value(); }

  SourceVarRef &operator=(ValueType newvalue) {
    set_value(std::move(newvalue));
    return *this;
  }

  template <typename U>
    requires requires(ValueType &t, const U &u) { t += u; }
  SourceVarRef &operator+=(const U &other) {
    ValueType new_value = BaseType::value();
    new_value += other;
    set_value(std::move(new_value));
    return *this;
  }

  template <typename U>
    requires requires(ValueType &t, const U &u) { t -= u; }
  SourceVarRef &operator-=(const U &other) {
    ValueType new_value = BaseType::value();
    new_value -= other;
    set_value(std::move(new_value));
    return *this;
  }

  template <typename U>
    requires requires(ValueType &t, const U &u) { t *= u; }
  SourceVarRef &operator*=(const U &other) {
    ValueType new_value = BaseType::value();
    new_value *= other;
    set_value(std::move(new_value));
    return *this;
  }

  template <typename U>
    requires requires(ValueType &t, const U &u) { t /= u; }
  SourceVarRef &operator/=(const U &other) {
    ValueType new_value = BaseType::value();
    new_value /= other;
    set_value(std::move(new_value));
    return *this;
  }

  SourceVarRef &operator++()
    requires requires(ValueType &t) { ++t; }
  {
    ValueType new_value = BaseType::value();
    ++new_value;
    set_value(std::move(new_value));
    return *this;
  }

  SourceVarRef &operator--()
    requires requires(ValueType &t) { --t; }
  {
    ValueType new_value = BaseType::value();
    --new_value;
    set_value(std::move(new_value));
    return *this;
  }

  ValueType operator++(int)
    requires requires(ValueType &t) { t++; }
  {
    ValueType old_value = BaseType::value();
    ValueType new_value = old_value;
    ++new_value;
    set_value(std::move(new_value));
    return old_value;
  }

  ValueType operator--(int)
    requires requires(ValueType &t) { t--; }
  {
    ValueType old_value = BaseType::value();
    ValueType new_value = old_value;
    --new_value;
    set_value(std::move(new_value));
    return old_value;
  }
};

template <typename TValue>
  requires std::copy_constructible<TValue> && EqualityComparable<TValue>
auto make_var_ref(TValue default_value) -> std::unique_ptr<VarRef<TValue>> {
  return std::unique_ptr<VarRef<TValue>>(
      new VarRef<TValue>(std::move(default_value)));
}

template <typename TValue>
  requires std::copy_constructible<TValue> && EqualityComparable<TValue>
auto make_source_var_ref(TValue default_value)
    -> std::unique_ptr<SourceVarRef<TValue>> {
  return std::make_unique<SourceVarRef<TValue>>(std::move(default_value));
}

template <typename TValue>
  requires std::copy_constructible<TValue> && EqualityComparable<TValue>
auto make_var_ref(TValue default_value,
                  std::function<bool(const TValue &, const TValue &)> comparer)
    -> std::unique_ptr<VarRef<TValue>> {
  return std::unique_ptr<VarRef<TValue>>(
      new VarRef<TValue>(std::move(default_value), std::move(comparer)));
}

template <typename TValue>
  requires std::copy_constructible<TValue> && EqualityComparable<TValue>
auto make_source_var_ref(
    TValue default_value,
    std::function<bool(const TValue &, const TValue &)> comparer)
    -> std::unique_ptr<SourceVarRef<TValue>> {
  return std::make_unique<SourceVarRef<TValue>>(std::move(default_value),
                                                std::move(comparer));
}
