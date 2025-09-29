#include "Requirements/RequirementMonitor.hpp"
#include "VarRef/VarRef.hpp"
#include <functional>

template<typename TValue>
using Compare = std::function<bool(const TValue&, const TValue&)>;

template<typename TValue>
class VarRefCompareMonitor : public RequirementMonitor {
private:
    VarRef<TValue> *left_{nullptr};
    VarRef<TValue> *right_{nullptr};
    Compare<TValue> check_;
    TValue right_value_;
    bool has_right_ref_;
    void OnValueChanged(const VarRef<TValue>& source, [[maybe_unused]]const TValue& old_value, const TValue& new_value) {
         const TValue& left_value = (&source == left_) ? new_value : left_->value();
         const TValue& right_value = (&source == left_) ? (has_right_ref_ ? right_->value() : right_value_) : new_value;
         set_fulfilled(check_(left_value, right_value));
    }
    typename VarRef<TValue>::ValueSubscribe left_subscribe_{};
    typename VarRef<TValue>::ValueSubscribe right_subscribe_{};
protected:
    void on_start() override {
       left_subscribe_ = left_->subscribe_value_changed([this](const VarRef<TValue> & source, const TValue & old_value, const TValue & new_value) {
            this->OnValueChanged(source, old_value, new_value);
       });
       if (has_right_ref_) {
        right_subscribe_ = right_->subscribe_value_changed([this](const VarRef<TValue> & source, const TValue & old_value, const TValue & new_value) {
            this->OnValueChanged(source, old_value, new_value);
       });
       }
       set_fulfilled(check_(left_->value(), has_right_ref_ ? right_->value() : right_value_));
    }
    void on_stop() override {
        left_subscribe_ = {};
        if (has_right_ref_) {
            right_subscribe_ = {};
        }
    }
public:
    VarRefCompareMonitor(VarRef<TValue>& left, VarRef<TValue>& right, Compare<TValue> check) 
        : left_(&left), right_(&right), check_(std::move(check)),
          right_value_(right.default_value()), has_right_ref_(true) {
    }
    VarRefCompareMonitor(VarRef<TValue>& left, TValue right_value,  Compare<TValue> check) 
        : left_(&left), check_(std::move(check)), 
          right_value_(right_value), has_right_ref_(false) {
    }
};