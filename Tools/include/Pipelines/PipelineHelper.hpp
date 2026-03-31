#pragma once
#include <functional>
#include <future>

enum class PipelineSlimHandleResult { Continue, Complete, Abort };

template <typename T>
using PipelineSlimHandler =
    std::function<std::future<PipelineSlimHandleResult>(T)>;

enum class RoutedPipelineHandleResult {
  Continue,
  StageNodeComplete,
  StageComplete,
  PipelineComplete,
  Abort
};

enum class ProcessStage { Pre, Target, Post };

template <typename T>
using RoutedPipelineHandler =
    std::function<std::future<RoutedPipelineHandleResult>(T)>;
