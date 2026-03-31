#pragma once

#include "Pipelines/PipelineHelper.hpp"
#include <map>
#include <set>

template <typename T> class PipelineSlim {
public:
  using Token = int;

  PipelineSlim() = default;
  PipelineSlim(const PipelineSlim &) = delete;
  PipelineSlim(PipelineSlim &&) = delete;
  PipelineSlim &operator=(const PipelineSlim &) = delete;
  PipelineSlim &operator=(PipelineSlim &&) = delete;

  Token registHandler(PipelineSlimHandler<T> handler, int priority = 0) {
    if (!handler)
      return -1;

    Token token = id_++;
    index_[token] = {priority, token};
    ordered_.insert({priority, token, std::move(handler)});
    return token;
  }

  void unregistHandler(Token token) {
    auto it = index_.find(token);
    if (it == index_.end())
      return;

    auto [priority, id] = it->second;
    ordered_.erase({priority, id, nullptr});
    index_.erase(it);
  }

  bool contains(Token token) const { return index_.contains(token); }

  std::future<bool> produce(T pack) {
    return std::async(std::launch::async, [this, pack]() -> bool {
      for (const auto &entry : ordered_) {
        auto result = entry.handler(pack).get();
        if (result == PipelineSlimHandleResult::Abort)
          return false;
        if (result == PipelineSlimHandleResult::Complete)
          return true;
      }
      return true;
    });
  }

private:
  struct Entry {
    int priority;
    int id;
    PipelineSlimHandler<T> handler;

    bool operator<(const Entry &o) const {
      if (priority != o.priority)
        return priority > o.priority;
      return id < o.id;
    }
  };

  static inline int id_{0};
  std::set<Entry> ordered_;
  std::map<Token, std::pair<int, int>> index_;
};
