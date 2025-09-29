#pragma once
#include "Requirements/RequirementMonitor.hpp"
#include "VarRef/VarRef.hpp"
#include <memory>

template <typename TValue>
using Compare = std::function<bool(const TValue &, const TValue &)>;

template <typename TValue>
class VarRefCompareMonitor : public RequirementMonitor {
private:
  VarRef<TValue> *left_{nullptr};
  VarRef<TValue> *right_{nullptr};
  Compare<TValue> check_;
  TValue right_value_;
  bool has_right_ref_;
  void OnValueChanged(const VarRef<TValue> &source, [[maybe_unused]]const TValue &old_value,
                      const TValue &new_value) {
    const TValue &left_value = (&source == left_) ? new_value : left_->value();
    const TValue &right_value =
        (&source == left_) ? (has_right_ref_ ? right_->value() : right_value_)
                           : new_value;
    set_fulfilled(check_(left_value, right_value));
  }
  typename VarRef<TValue>::ValueSubscribe left_subscribe_{};
  typename VarRef<TValue>::ValueSubscribe right_subscribe_{};

protected:
  void on_start() override {
    left_subscribe_ = left_->subscribe_value_changed(
        [this](const VarRef<TValue> &source, const TValue &old_value,
               const TValue &new_value) {
          this->OnValueChanged(source, old_value, new_value);
        });
    if (has_right_ref_) {
      right_subscribe_ = right_->subscribe_value_changed(
          [this](const VarRef<TValue> &source, const TValue &old_value,
                 const TValue &new_value) {
            this->OnValueChanged(source, old_value, new_value);
          });
    }
    set_fulfilled(check_(left_->value(),
                         has_right_ref_ ? right_->value() : right_value_));
  }
  void on_stop() override {
    left_subscribe_ = {};
    if (has_right_ref_) {
      right_subscribe_ = {};
    }
  }

public:
  VarRefCompareMonitor(VarRef<TValue> &left, VarRef<TValue> &right,
                       Compare<TValue> check)
      : left_(&left), right_(&right), check_(std::move(check)),
        right_value_(right.default_value()), has_right_ref_(true) {}
  VarRefCompareMonitor(VarRef<TValue> &left, const TValue &right_value,
                       Compare<TValue> check)
      : left_(&left), check_(std::move(check)), right_value_(right_value),
        has_right_ref_(false) {}
};

template <typename TValue>
std::unique_ptr<RequirementMonitor> BiggerThan(VarRef<TValue> &left,
                                               VarRef<TValue> &right) {
  return std::make_unique<VarRefCompareMonitor<TValue>>(
      left, right, [](const TValue &l_value, const TValue &r_value) {
        return l_value > r_value;
      });
}
template <typename TValue>
std::unique_ptr<RequirementMonitor> BiggerThan(VarRef<TValue> &left,
                                               const TValue &right) {
  return std::make_unique<VarRefCompareMonitor<TValue>>(
      left, right, [](const TValue &l_value, const TValue &r_value) {
        return l_value > r_value;
      });
}

template <typename TValue>
std::unique_ptr<RequirementMonitor> LessThan(VarRef<TValue> &left,
                                             VarRef<TValue> &right) {
  return std::make_unique<VarRefCompareMonitor<TValue>>(
      left, right, [](const TValue &l_value, const TValue &r_value) {
        return l_value < r_value;
      });
}
template <typename TValue>
std::unique_ptr<RequirementMonitor> LessThan(VarRef<TValue> &left,
                                             const TValue &right) {
  return std::make_unique<VarRefCompareMonitor<TValue>>(
      left, right, [](const TValue &l_value, const TValue &r_value) {
        return l_value < r_value;
      });
}

template <typename TValue>
std::unique_ptr<RequirementMonitor> NoBiggerThan(VarRef<TValue> &left,
                                                 VarRef<TValue> &right) {
  return std::make_unique<VarRefCompareMonitor<TValue>>(
      left, right, [](const TValue &l_value, const TValue &r_value) {
        return l_value <= r_value;
      });
}
template <typename TValue>
std::unique_ptr<RequirementMonitor> NoBiggerThan(VarRef<TValue> &left,
                                                 const TValue &right) {
  return std::make_unique<VarRefCompareMonitor<TValue>>(
      left, right, [](const TValue &l_value, const TValue &r_value) {
        return l_value <= r_value;
      });
}

template <typename TValue>
std::unique_ptr<RequirementMonitor> NoLessThan(VarRef<TValue> &left,
                                               VarRef<TValue> &right) {
  return std::make_unique<VarRefCompareMonitor<TValue>>(
      left, right, [](const TValue &l_value, const TValue &r_value) {
        return l_value >= r_value;
      });
}
template <typename TValue>
std::unique_ptr<RequirementMonitor> NoLessThan(VarRef<TValue> &left,
                                               const TValue &right) {
  return std::make_unique<VarRefCompareMonitor<TValue>>(
      left, right, [](const TValue &l_value, const TValue &r_value) {
        return l_value >= r_value;
      });
}

template <typename TValue>
std::unique_ptr<RequirementMonitor> EqualTo(VarRef<TValue> &left,
                                            VarRef<TValue> &right) {
  return std::make_unique<VarRefCompareMonitor<TValue>>(
      left, right,
      [&left, &right](const TValue &l_value, const TValue &r_value) {
        if (left.same_comparer(right)) {
          return left.Equals(l_value, r_value);
        }
        return l_value == r_value;
      });
}
template <typename TValue>
std::unique_ptr<RequirementMonitor> EqualTo(VarRef<TValue> &left,
                                            const TValue &right) {
  return std::make_unique<VarRefCompareMonitor<TValue>>(
      left, right, [&left](const TValue &l_value, const TValue &r_value) {
        return left.Equals(l_value, r_value);
      });
}

template <typename TValue>
std::unique_ptr<RequirementMonitor>
CompareTo(VarRef<TValue> &left, VarRef<TValue> &right, Compare<TValue> comp) {
  return std::make_unique<VarRefCompareMonitor<TValue>>(left, right,
                                                        std::move(comp));
}
template <typename TValue>
std::unique_ptr<RequirementMonitor>
CompareTo(VarRef<TValue> &left, const TValue &right, Compare<TValue> comp) {
  return std::make_unique<VarRefCompareMonitor<TValue>>(left, right,
                                                        std::move(comp));
}
