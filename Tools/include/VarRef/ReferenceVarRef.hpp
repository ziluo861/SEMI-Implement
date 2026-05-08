#pragma once

#include "VarRef/VarRef.hpp"
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {
template <typename Cmp, typename T>
static int tri_cmp(const Cmp &cmp, const T &a, const T &b) {
  if (cmp(a, b))
    return -1;
  if (cmp(b, a))
    return 1;
  return 0;
}
template <typename T, typename Comparator>
static int Compare(const T &left, const T &right, bool is_min,
                   const Comparator &com) {
  const int c = tri_cmp(com, left, right);
  return is_min ? c : -c;
}
} // namespace

template <typename TValue> class ReferenceVarRef : public VarRef<TValue> {
private:
  using BaseType = VarRef<TValue>;
  using ComparerType = typename BaseType::ComparerType;
  using ValueSubscribe = typename BaseType::ValueSubscribe;
  static constexpr std::size_t kMinMaxHeapSlimThreshold = 127;
  template <class T>
  static std::vector<VarRef<T> *>
  dedup_sources(const std::vector<VarRef<T> *> &in) {
    std::vector<VarRef<T> *> out;
    out.reserve(in.size());
    std::unordered_set<VarRef<T> *> seen;
    seen.reserve(in.size());
    for (auto *p : in) {
      if (p && seen.insert(p).second) // 保留首见，过滤 nullptr
        out.push_back(p);
    }
    return out;
  }
  void OnValueChange(const TValue &value) { this->set_value(value); }
  struct IBinderBase {
    virtual ~IBinderBase() = default;
    virtual bool OwnsSource([[maybe_unused]] const void *p) const noexcept {
      return false;
    }
  };
  struct BinderToken {
    void *self = nullptr;
    void (*unsubscribe)(void *, std::size_t) = nullptr;
    std::size_t id = 0;
    BinderToken() = default;
    BinderToken(void *s, void (*u)(void *, std::size_t), std::size_t i)
        : self(s), unsubscribe(u), id(i) {}
    BinderToken(BinderToken &&o) noexcept
        : self(std::exchange(o.self, nullptr)),
          unsubscribe(std::exchange(o.unsubscribe, nullptr)),
          id(std::exchange(o.id, 0)) {}
    BinderToken &operator=(BinderToken &&o) noexcept {
      if (this != &o) {
        reset();
        self = o.self;
        unsubscribe = o.unsubscribe;
        id = o.id;
        o.self = nullptr;
        o.unsubscribe = nullptr;
        o.id = 0;
      }
      return *this;
    }
    ~BinderToken() { reset(); }
    void reset() noexcept {
      if (self && unsubscribe && id) {
        unsubscribe(self, id);
        self = nullptr;
        id = 0;
      }
    }
    BinderToken(const BinderToken &) = delete;
    BinderToken &operator=(const BinderToken &) = delete;
  };
  template <typename Derived> class IBinder : public IBinderBase {
  private:
    using CallbackType = std::function<void(const TValue &)>;
    std::vector<std::pair<std::size_t, CallbackType>> value_changed_callbacks_;
    std::size_t id_ = 0;
    bool destroyed_ = false;

  public:
    virtual ~IBinder() {
      destroyed_ = true;
      value_changed_callbacks_.clear();
    }
    const std::vector<std::pair<std::size_t, CallbackType>> &
    ValueChanged() const noexcept {
      return value_changed_callbacks_;
    }
    [[nodiscard]]
    BinderToken BindSubscribe(CallbackType callback) {
      if (destroyed_) {
        throw std::runtime_error("IBinder was destroyed!!");
      }
      const std::size_t _size = ++id_;
      value_changed_callbacks_.emplace_back(_size, std::move(callback));
      return BinderToken{
          this,
          [](void *self, std::size_t id) {
            static_cast<IBinder<Derived> *>(self)->UnBindSubscribe(id);
          },
          _size};
    }
    void UnBindSubscribe(std::size_t id) {
      if (value_changed_callbacks_.empty() || destroyed_)
        return;
      auto it = std::find_if(
          value_changed_callbacks_.begin(), value_changed_callbacks_.end(),
          [this, id](const auto &p) { return p.first == id; });
      if (it != value_changed_callbacks_.end()) {
        value_changed_callbacks_.erase(it);
      }
    }
    bool Destroyed() const noexcept { return destroyed_; }
  };
  template <typename T> class Binder : public IBinder<Binder<T>> {
  private:
    using BaseType = IBinder<Binder<T>>;
    VarRef<T> *source_;
    std::vector<ValueSubscribe> binder_subscribes_;
    void OnValueChanged(const VarRef<T> &, const T &, const T &new_value) {
      auto callbacks = BaseType::ValueChanged();
      for (auto &[_, callback] : callbacks) {
        if (BaseType::Destroyed())
          break;
        callback(new_value);
      }
    }

  public:
    static_assert(std::is_same_v<T, TValue>,
                  "Single-source Binder<T> must match ReferenceVarRef<TValue>");
    bool OwnsSource(const void *p) const noexcept override {
      return p == source_;
    }
    explicit Binder(VarRef<T> *source) : source_(source) {
      auto cb = source_->subscribe_value_changed(
          [this](const VarRef<T> &var_ref, const T &old_value,
                 const T &new_value) {
            OnValueChanged(var_ref, old_value, new_value);
          });
      binder_subscribes_.emplace_back(std::move(cb));
    }
    ~Binder() { binder_subscribes_.clear(); }
  };

  template <class F, class... Ts>
  class FunctionBinder : public IBinder<FunctionBinder<F, Ts...>> {
    using BaseB = IBinder<FunctionBinder<F, Ts...>>;
    using SubsTuple = std::tuple<typename VarRef<Ts>::ValueSubscribe...>;
    std::tuple<VarRef<Ts> *...> sources_;
    std::tuple<Ts...> cache_;
    F func_;
    SubsTuple subs_;

    template <std::size_t I> void AttachOne() {
      using Ti = std::tuple_element_t<I, std::tuple<Ts...>>;
      auto *s = std::get<I>(sources_);
      std::get<I>(subs_) = s->subscribe_value_changed(
          [this](const VarRef<Ti> &, const Ti &, const Ti &nv) {
            std::get<I>(cache_) = nv;
            TValue out = std::apply(
                [this](auto &...xs) -> TValue {
                  using R = std::invoke_result_t<F &, decltype(xs)...>;
                  static_assert(std::is_convertible_v<R, TValue>,
                                "func result must be convertible to TValue");
                  return static_cast<TValue>(std::invoke(func_, xs...));
                },
                cache_);
            auto callbacks = BaseB::ValueChanged();
            for (auto &[_, cb] : callbacks) {
              if (BaseB::Destroyed())
                break;
              cb(out);
            }
          });
    }

    template <std::size_t... Is> void AttachAll(std::index_sequence<Is...>) {
      (AttachOne<Is>(), ...);
    }

  public:
    bool OwnsSource(const void *p) const noexcept override {
      bool found = false;
      std::apply([&](auto *...ps) { ((found = found || (p == ps)), ...); },
                 sources_);
      return found;
    }

    FunctionBinder(F func, VarRef<Ts> *...srcs)
        : sources_(srcs...), cache_(srcs->value()...), func_(std::move(func)) {
      AttachAll(std::index_sequence_for<Ts...>{});
    }

    ~FunctionBinder() {}
  };

  template <typename T, typename TSource>
    requires requires(T a, T b) {
      { a + b } -> std::convertible_to<T>;
      { a - b } -> std::convertible_to<T>;
    }
  class AggregateBinder : public IBinder<AggregateBinder<T, TSource>> {
  private:
    using BaseType = IBinder<AggregateBinder<T, TSource>>;
    std::function<T(const TSource &)> selector_;
    std::vector<VarRef<TSource> *> sources_;
    T value_;
    std::vector<typename VarRef<TSource>::ValueSubscribe> aggregate_subscribes_;
    void OnValueChanged(const VarRef<TSource> &, const TSource &old_value,
                        const TSource &new_value) {
      auto callbacks = BaseType::ValueChanged();
      for (auto &[_, callback] : callbacks) {
        if (BaseType::Destroyed())
          break;
        value_ = value_ + selector_(new_value) - selector_(old_value);
        callback(value_);
      }
    }

  public:
    static_assert(
        std::is_same_v<TSource, TValue>,
        "AggregateBinder<T, TSource> must match ReferenceVarRef<TValue>");
    bool OwnsSource(const void *p) const noexcept override {
      for (auto *s : sources_)
        if (s == p)
          return true;
      return false;
    }
    explicit AggregateBinder(T seed, std::function<T(const TSource &)> selector,
                             const std::vector<VarRef<TSource> *> &source)
        : value_(seed), selector_(std::move(selector)), sources_(source) {
      for (auto *it : sources_) {
        auto cb = it->subscribe_value_changed(
            [this](const VarRef<TSource> &var_ref, const TSource &old_value,
                   const TSource &new_value) {
              OnValueChanged(var_ref, old_value, new_value);
            });
        aggregate_subscribes_.emplace_back(std::move(cb));
      }
    }
    ~AggregateBinder() { aggregate_subscribes_.clear(); }
  };

  template <typename T, typename Comparator>
  class MinMaxBinderSlim : public IBinder<MinMaxBinderSlim<T, Comparator>> {
  private:
    using BaseType = IBinder<MinMaxBinderSlim<T, Comparator>>;
    Comparator comparison_;
    std::vector<VarRef<T> *> sources_;
    bool is_min_;
    T value_;
    std::vector<ValueSubscribe> slim_subscribes_;
    void OnValueChanged(const VarRef<T> &, const T &old_value,
                        const T &new_value) {
      if (Compare(old_value, new_value, is_min_, comparison_) > 0) {
        if (Compare(value_, new_value, is_min_, comparison_) <= 0)
          return;
        value_ = new_value;
        auto callbacks = BaseType::ValueChanged();
        for (auto &[_, cb] : callbacks) {
          if (BaseType::Destroyed())
            break;
          cb(value_);
        }
        return;
      }
      if (Compare(value_, old_value, is_min_, comparison_) >= 0) {
        T best = sources_.empty() ? value_ : sources_.front()->value();
        for (std::size_t i = 1; i < sources_.size(); ++i) {
          const auto v = sources_[i]->value();
          if (Compare(v, best, is_min_, comparison_) < 0)
            best = v;
        }
        if (Compare(best, value_, is_min_, comparison_) != 0) {
          value_ = best;
          auto callbacks = BaseType::ValueChanged();
          for (auto &[_, cb] : callbacks) {
            if (BaseType::Destroyed())
              break;
            cb(value_);
          }
        }
      }
    }

  public:
    static_assert(
        std::is_same_v<T, TValue>,
        "MinMaxBinderSlim<T, Comparator> must match ReferenceVarRef<TValue>");
    bool OwnsSource(const void *p) const noexcept override {
      for (auto *s : sources_)
        if (s == p)
          return true;
      return false;
    }
    void Set_Value(const T &value) { value_ = value; }
    T Value() const noexcept { return value_; }
    explicit MinMaxBinderSlim(bool is_min, T value, Comparator comparison,
                              const std::vector<VarRef<T> *> &source)
        : is_min_(is_min), comparison_(std::move(comparison)),
          sources_(source) {
      if (sources_.empty()) {
        value_ = value;
      } else {
        value_ = sources_.front()->value();
        for (std::size_t i = 1; i < sources_.size(); ++i) {
          const auto v = sources_[i]->value();
          if (Compare(v, value_, is_min_, comparison_) < 0)
            value_ = v;
        }
      }
      for (auto *it : sources_) {
        auto cb = it->subscribe_value_changed([this](const VarRef<T> &var_ref,
                                                     const T &old_value,
                                                     const T &new_value) {
          OnValueChanged(var_ref, old_value, new_value);
        });
        slim_subscribes_.emplace_back(std::move(cb));
      }
    }
    ~MinMaxBinderSlim() { slim_subscribes_.clear(); }
  };

  template <typename T, typename Comparator>
  class MinMaxBinder : public IBinder<MinMaxBinder<T, Comparator>> {
  private:
    using BaseType = IBinder<MinMaxBinder<T, Comparator>>;
    Comparator comparison_;
    bool is_min_;
    std::vector<VarRef<T> *> heap_;
    std::unordered_map<VarRef<T> *, std::size_t> indexs_;
    std::vector<ValueSubscribe> subscribes_;
    void OnValueChanged(const VarRef<T> &varRef, const T &old_Value,
                        const T &new_Value) {
      auto comp = Compare(old_Value, new_Value, is_min_, comparison_);
      if (comp == 0)
        return;

      auto it = indexs_.find(const_cast<VarRef<T> *>(&varRef));
      if (it == indexs_.end())
        return;
      std::size_t n = it->second;

      if (comp > 0) {
        // 变“更优”（min: 变小 / max: 变大）→ 上滤
        auto *target = heap_[n];
        for (std::size_t n2 = (n - 1) >> 1;
             n > 0 && Compare(target->value(), heap_[n2]->value(), is_min_,
                              comparison_) < 0;
             n2 = ((n = n2) - 1) >> 1) {
          indexs_[heap_[n] = heap_[n2]] = n;
        }
        indexs_[heap_[n] = target] = n;
        if (n != 0)
          return; // 未到顶无需通知
                  // 到顶，值肯定发生了“更优”变化，直接通知
      } else {
        // 变“更差”（min: 变大 / max: 变小）→ 下滤
        const bool nottop = (n != 0);
        auto *target = heap_[n];
        for (std::size_t n2 = (n << 1) + 1, len = heap_.size(); n2 < len;
             n2 = ((n = n2) << 1) + 1) {
          if (n2 + 1 < len &&
              Compare(heap_[n2 + 1]->value(), heap_[n2]->value(), is_min_,
                      comparison_) < 0)
            ++n2;
          if (Compare(heap_[n2]->value(), target->value(), is_min_,
                      comparison_) >= 0)
            break;
          indexs_[heap_[n] = heap_[n2]] = n;
        }
        indexs_[heap_[n] = target] = n;
        if (nottop)
          return; // 非顶点无须通知

        // 顶点可能换人，但顶点“值”不一定变；若没变则不通知
        const T &new_top = heap_[0]->value();
        if (tri_cmp(comparison_, new_top, old_Value) == 0)
          return;
      }

      auto callbacks = BaseType::ValueChanged();
      for (auto &[_, callback] : callbacks) {
        if (BaseType::Destroyed())
          break;
        callback(heap_[0]->value());
      }
    }

  public:
    static_assert(
        std::is_same_v<T, TValue>,
        "MinMaxBinder<T, Comparator> must match ReferenceVarRef<TValue>");
    bool OwnsSource(const void *p) const noexcept override {
      return indexs_.find(reinterpret_cast<VarRef<T> *>(
                 const_cast<void *>(p))) != indexs_.end();
    }
    ~MinMaxBinder() { subscribes_.clear(); }
    explicit MinMaxBinder(bool is_min, Comparator comparison,
                          const std::vector<VarRef<T> *> &heap)
        : is_min_(is_min), comparison_(std::move(comparison)), heap_(heap) {
      for (std::size_t i = 0; i < heap_.size(); i++) {
        auto cb = heap_[i]->subscribe_value_changed(
            [this](const VarRef<T> &var_ref, const T &old_value,
                   const T &new_value) {
              OnValueChanged(var_ref, old_value, new_value);
            });
        subscribes_.emplace_back(std::move(cb));
      }
      for (std::size_t i = 0; i < heap_.size(); ++i)
        indexs_[heap_[i]] = i;
      for (std::size_t i = heap_.size() / 2; i-- > 0;) {
        std::size_t n = i;
        auto *target = heap_[n];
        for (std::size_t n2 = (n << 1) + 1, len = heap_.size(); n2 < len;
             n2 = ((n = n2) << 1) + 1) {
          if (n2 + 1 < len &&
              Compare(heap_[n2 + 1]->value(), heap_[n2]->value(), is_min_,
                      comparison_) < 0)
            ++n2;
          if (Compare(heap_[n2]->value(), target->value(), is_min_,
                      comparison_) >= 0)
            break;
          indexs_[heap_[n] = heap_[n2]] = n;
        }
        indexs_[heap_[n] = target] = n;
      }
    }
  };

private:
  std::unique_ptr<IBinderBase> binder_;
  std::optional<BinderToken> subscribe_;
  void UnBind() {
    if (!binder_)
      return;
    subscribe_.reset();
    binder_.reset();
  }

public:
  ReferenceVarRef(TValue default_value, ComparerType com = {})
      : BaseType(default_value, std::move(com)) {}
  ~ReferenceVarRef() { UnBind(); }
  bool Bind(std::function<
            std::tuple<std::unique_ptr<IBinderBase>, BinderToken, TValue>()>
                factory) {
    if (binder_)
      return false;
    auto [b, tok, init] = factory();
    subscribe_.emplace(std::move(tok));
    binder_ = std::move(b);
    this->set_value(init);
    return true;
  }
  bool UnBind(VarRef<TValue> *var_ref) {
    if (!binder_ || !binder_->OwnsSource(static_cast<const void *>(var_ref))) {
      return false;
    }
    UnBind();
    return true;
  }
  bool UnBind(VarRef<TValue> *var_ref, TValue value) {
    if (!UnBind(var_ref))
      return false;
    this->set_value(value);
    return true;
  }
  bool BindTo(VarRef<TValue> &src) {
    return Bind([&] {
      using B = Binder<TValue>;
      auto b = std::make_unique<B>(&src);
      auto tok =
          b->BindSubscribe([this](const TValue &v) { OnValueChange(v); });
      return std::make_tuple(std::unique_ptr<IBinderBase>(std::move(b)),
                             std::move(tok), src.value());
    });
  }

  template <class F, class... Ts> bool BindFunc(F &&func, VarRef<Ts> &...srcs) {
    using Fn = std::decay_t<F>;
    static_assert(std::is_invocable_r_v<TValue, Fn, Ts...>,
                  "func signature must be TValue(Ts...)");
    Fn fn(std::forward<F>(func));
    return Bind([&] {
      using B = FunctionBinder<Fn, Ts...>;
      auto b = std::make_unique<B>(fn, (&srcs)...);
      auto tok =
          b->BindSubscribe([this](const TValue &v) { OnValueChange(v); });
      TValue init = static_cast<TValue>(std::invoke(fn, srcs.value()...));
      return std::make_tuple(std::unique_ptr<IBinderBase>(std::move(b)),
                             std::move(tok), init);
    });
  }

  template <class TSource, class Selector>
  bool BindAggregate(TValue seed, Selector selector,
                     const std::vector<VarRef<TSource> *> &sources) {
    auto uniq = dedup_sources(sources);
    return Bind([&] {
      using B = AggregateBinder<TValue, TSource>;
      TValue init = seed;
      for (auto *s : uniq)
        init = init + selector(s->value());
      auto b = std::make_unique<B>(
          init, std::function<TValue(const TSource &)>(selector), uniq);
      auto tok =
          b->BindSubscribe([this](const TValue &v) { OnValueChange(v); });
      return std::make_tuple(std::unique_ptr<IBinderBase>(std::move(b)),
                             std::move(tok), init);
    });
  }

  template <class Cmp = std::less<TValue>>
  bool BindMin(Cmp cmp, const std::vector<VarRef<TValue> *> &sources) {
    return BindMinMaxAuto(/*is_min*/ true, std::move(cmp), sources,
                          kMinMaxHeapSlimThreshold);
  }

  template <class Cmp = std::less<TValue>>
  bool BindMax(Cmp cmp, const std::vector<VarRef<TValue> *> &sources) {
    return BindMinMaxAuto(/*is_min*/ false, std::move(cmp), sources,
                          kMinMaxHeapSlimThreshold);
  }

  template <class Cmp = std::less<TValue>>
  bool BindMinMaxAuto(bool is_min, Cmp cmp,
                      const std::vector<VarRef<TValue> *> &sources,
                      std::size_t slim_threshold = kMinMaxHeapSlimThreshold) {
    auto uniq = dedup_sources(sources);
    if (uniq.size() > slim_threshold) {
      return Bind([&] {
        using B = MinMaxBinder<TValue, Cmp>;
        TValue init = uniq.empty() ? this->value() : uniq.front()->value();
        for (size_t i = 1; i < uniq.size(); ++i) {
          if (is_min ? cmp(uniq[i]->value(), init)
                     : cmp(init, uniq[i]->value()))
            init = uniq[i]->value();
        }
        auto b = std::make_unique<B>(is_min, std::move(cmp), uniq);
        auto tok =
            b->BindSubscribe([this](const TValue &v) { this->set_value(v); });
        return std::make_tuple(std::unique_ptr<IBinderBase>(std::move(b)),
                               std::move(tok), init);
      });
    } else {
      return Bind([&] {
        using B = MinMaxBinderSlim<TValue, Cmp>;
        TValue init = uniq.empty() ? this->value() : uniq.front()->value();
        for (size_t i = 1; i < uniq.size(); ++i)
          if (is_min ? (tri_cmp(cmp, uniq[i]->value(), init) < 0)
                     : (tri_cmp(cmp, init, uniq[i]->value()) < 0))
            init = uniq[i]->value();
        auto b =
            std::make_unique<B>(is_min, this->value(), std::move(cmp), uniq);
        auto tok =
            b->BindSubscribe([this](const TValue &v) { OnValueChange(v); });
        return std::make_tuple(std::unique_ptr<IBinderBase>(std::move(b)),
                               std::move(tok), init);
      });
    }
  }
};
