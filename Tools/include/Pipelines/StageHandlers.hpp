#pragma once
#include "Pipelines/PipelineHelper.hpp"
#include <map>
#include <set>

template <typename T> class StageHandlers {
public:
  using Token = int;

  Token regist(RoutedPipelineHandler<T> handler, int priority = 0) {
    if (!handler)
      return -1;
    Token token = nextId_++;
    index_[token] = {priority, token};
    ordered_.insert({priority, token, std::move(handler)});
    return token;
  }

  void unregist(Token token) {
    auto it = index_.find(token);
    if (it == index_.end())
      return;
    auto [priority, id] = it->second;
    ordered_.erase({priority, id, nullptr});
    index_.erase(it);
  }

  bool contains(Token token) const { return index_.contains(token); }

  const auto &handlers() const { return ordered_; }

private:
  struct Entry {
    int priority;
    int id;
    RoutedPipelineHandler<T> handler;
    bool operator<(const Entry &o) const {
      if (priority != o.priority)
        return priority > o.priority;
      return id < o.id;
    }
  };

  inline static int nextId_{0};
  std::set<Entry> ordered_;
  std::map<Token, std::pair<int, int>> index_;
};
