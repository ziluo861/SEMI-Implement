#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T> class queue {
  // using T = int;
private:
  struct Node {
    std::atomic<Node *> next_{nullptr};
    std::optional<T> data_;
    Node(T &&data) : data_(std::move(data)) {}
    Node() = default;
  };
  struct HazardManager {
  private:
    struct alignas(64) Hazard {
      std::atomic<void *> p{nullptr};
    };
    std::vector<std::uintptr_t> get_snapshot() {
      std::vector<std::uintptr_t> snap_shot;
      std::size_t maxsize = KhpPerThreads_ * KMaxThreads_;
      snap_shot.reserve(maxsize);
      for (std::size_t i = 0; i < maxsize; i++) {
        void *ptr = hp_tables_[i].p.load(std::memory_order_acquire);
        if (ptr)
          snap_shot.push_back(reinterpret_cast<std::uintptr_t>(ptr));
      }
      std::sort(snap_shot.begin(), snap_shot.end());
      snap_shot.erase(std::unique(snap_shot.begin(), snap_shot.end()),
                      snap_shot.end());
      return snap_shot;
    }
    void retires_with_snap(std::vector<std::uintptr_t> &snap,
                           std::vector<Node *> &retire) {
      auto keep_to_erase = [&snap](Node *n) {
        const auto addr = reinterpret_cast<std::uintptr_t>(n);
        const bool is_deleted =
            !std::binary_search(snap.begin(), snap.end(), addr);
        if (is_deleted) {
          delete n;
          return true;
        }
        return false;
      };
      std::erase_if(retire, keep_to_erase);
    }

    static std::size_t acquire_slot() {
      for (std::size_t i = 0; i < KMaxThreads_; ++i) {
        bool excepted = false;
        if (g_used_[i].used_.compare_exchange_strong(
                excepted, true, std::memory_order_acq_rel)) {
          return i * KhpPerThreads_;
        }
      }
      throw std::runtime_error("out of KMaxThreads_");
    }
    static void release_slot() {
      std::size_t base = hp_Base_;
      for (std::size_t i = 0; i < KhpPerThreads_; ++i) {
        hp_tables_[base + i].p.store(nullptr, std::memory_order_release);
      }
      std::size_t i = base / KhpPerThreads_;
      g_used_[i].used_.store(false, std::memory_order_release);
    }
    static constexpr std::size_t KMaxThreads_ = 128;
    static constexpr std::size_t KhpPerThreads_ = 2;
    static constexpr std::size_t KRetireThreshold_ = 64;
    static inline std::array<Hazard, KMaxThreads_ * KhpPerThreads_>
        hp_tables_{};
    static inline thread_local std::vector<Node *> retired_{};
    static inline std::mutex global_mutex_;
    static inline std::vector<Node *> global_retired_{};
    static inline thread_local std::size_t hp_Base_ = acquire_slot();
    struct alignas(64) SlotState {
      std::atomic<bool> used_{false};
    };
    static inline std::array<SlotState, KMaxThreads_> g_used_{};

    HazardManager() = default;
    ~HazardManager() {
      for (std::size_t i = 0; i < KhpPerThreads_ * KMaxThreads_; ++i) {
        hp_tables_[i].p.store(nullptr, std::memory_order_relaxed);
      }
      std::lock_guard<std::mutex> lock(global_mutex_);
      for (Node *p : global_retired_)
        delete p;
      global_retired_.clear();
    }

    HazardManager(const HazardManager &) = delete;
    HazardManager &operator=(const HazardManager &) = delete;
    HazardManager(HazardManager &&) = delete;
    HazardManager &operator=(HazardManager &&) = delete;

    void try_reclaim_nodes() {
      if (retired_.size() < KRetireThreshold_)
        return;
      std::vector<std::uintptr_t> snap_shot = get_snapshot();
      retires_with_snap(snap_shot, retired_);
      {
        std::lock_guard<std::mutex> lock(global_mutex_);
        if (!retired_.empty()) {
          global_retired_.insert(global_retired_.end(), retired_.begin(),
                                 retired_.end());
          retired_.clear();
        }
        retires_with_snap(snap_shot, global_retired_);
      }
    }

    struct TlsGuard {
      ~TlsGuard() {
        HazardManager::instance().try_reclaim_all_nodes();
        HazardManager::instance().release_slot();
      }
    };
    static inline thread_local TlsGuard tls_guard_{};

  public:
    static HazardManager &instance() {
      static HazardManager instance_;
      return instance_;
    }
    void set_hazard(std::size_t index, void *ptr) {
      (void)tls_guard_;
      hp_tables_[hp_Base_ + index].p.store(ptr, std::memory_order_release);
    }
    void unset_hazard(std::size_t index) {
      hp_tables_[hp_Base_ + index].p.store(nullptr, std::memory_order_release);
    }
    void try_reclaim_all_nodes() {
      std::vector<std::uintptr_t> snap_shot = get_snapshot();
      retires_with_snap(snap_shot, retired_);
      {
        std::lock_guard<std::mutex> lock(global_mutex_);
        if (!retired_.empty()) {
          global_retired_.insert(global_retired_.end(), retired_.begin(),
                                 retired_.end());
          retired_.clear();
        }
        retires_with_snap(snap_shot, global_retired_);
      }
    }
    void retired(Node *node) {
      (void)tls_guard_;
      retired_.push_back(node);
      try_reclaim_nodes();
    }
  };
  struct RetireGuard {
    Node *p_;
    explicit RetireGuard(Node *node) : p_(node) {}
    ~RetireGuard() {
      if (p_)
        HazardManager::instance().retired(p_);
    }
    void clear() { p_ = nullptr; }
  };
  struct SlotGuard {
    explicit SlotGuard(std::size_t index) : index_(index) {}
    void set(Node *node) {
      active_ = true;
      HazardManager::instance().set_hazard(index_, node);
    }
    void unset() {
      if (active_) {
        HazardManager::instance().unset_hazard(index_);
        active_ = false;
      }
    }
    ~SlotGuard() { unset(); }

  private:
    std::size_t index_;
    bool active_{false};
  };

public:
  alignas(64) std::atomic<Node *> head_;
  alignas(64) std::atomic<Node *> tail_;
  queue() {
    Node *dummy = new Node();
    head_.store(dummy, std::memory_order_relaxed);
    tail_.store(dummy, std::memory_order_relaxed);
  }

  queue(const queue &) = delete;
  queue &operator=(const queue &) = delete;
  queue(queue &&) = delete;
  queue &operator=(queue &&) = delete;
  ~queue() {
    HazardManager::instance().try_reclaim_all_nodes();
    Node *p = head_.load(std::memory_order_relaxed);
    while (p) {
      Node *q = p->next_.load(std::memory_order_relaxed);
      delete p;
      p = q;
    }
  }
  bool empty() {
    for (;;) {
      Node *head = head_.load(std::memory_order_acquire);
      SlotGuard head_guard(0);
      head_guard.set(head);
      if (head != head_.load(std::memory_order_acquire)) {
        continue;
      }

      Node *next = head->next_.load(std::memory_order_acquire);
      SlotGuard next_guard(1);
      next_guard.set(next);

      if (next != head->next_.load(std::memory_order_acquire)) {
        continue;
      }
      bool result = (next == nullptr);
      return result;
    }
  }
  void push(T val) {
    Node *node = new Node(std::move(val));
    for (;;) {
      Node *current_tail = tail_.load(std::memory_order_acquire);
      Node *next = current_tail->next_.load(std::memory_order_acquire);
      if (current_tail == tail_.load(std::memory_order_acquire)) {
        if (next == nullptr) {
          if (current_tail->next_.compare_exchange_weak(
                  next, node, std::memory_order_release,
                  std::memory_order_relaxed)) {
            tail_.compare_exchange_strong(current_tail, node,
                                          std::memory_order_acq_rel,
                                          std::memory_order_acquire);
            return;
          }
        } else {
          tail_.compare_exchange_weak(current_tail, next,
                                      std::memory_order_acq_rel,
                                      std::memory_order_acquire);
        }
      }
    }
  }
  static_assert(std::is_nothrow_move_constructible_v<T> &&
                    std::is_nothrow_move_assignable_v<T>,
                "queue<T> requires noexcept move for lock-free pop");
  bool pop(T &result) {
    for (;;) {
      Node *current_head = head_.load(std::memory_order_acquire);
      SlotGuard head_guard(0);
      head_guard.set(current_head);
      if (current_head != head_.load(std::memory_order_acquire)) {
        continue;
      }

      Node *next = current_head->next_.load(std::memory_order_acquire);
      SlotGuard next_guard(1);
      next_guard.set(next);
      if (next == nullptr) {
        return false;
      }
      if (next != current_head->next_.load(std::memory_order_acquire)) {
        continue;
      }

      Node *current_tail = tail_.load(std::memory_order_acquire);
      if (current_head == current_tail) {
        tail_.compare_exchange_weak(current_tail, next,
                                    std::memory_order_acq_rel,
                                    std::memory_order_acquire);
        continue;
      } else {
        if (head_.compare_exchange_strong(current_head, next,
                                          std::memory_order_acq_rel,
                                          std::memory_order_acquire)) {
          RetireGuard retire_node(current_head);
          result = std::move(*next->data_);
          next->data_.reset();
          return true;
        }
      }
    }
  }
};