/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#ifndef TENSORFLOW_COMPILER_XLA_SERVICE_GPU_GPU_GRAPH_UTIL_H_
#define TENSORFLOW_COMPILER_XLA_SERVICE_GPU_GPU_GRAPH_UTIL_H_

#include <atomic>
#include <memory>
#include <string>

#include "tensorflow/compiler/xla/service/gpu/buffer_allocations.h"
#include "tensorflow/compiler/xla/service/gpu/gpu_types.h"
#include "tensorflow/compiler/xla/statusor.h"
#include "tensorflow/core/platform/macros.h"
#include "tensorflow/core/platform/stream_executor_no_cuda.h"
#include "tensorflow/core/util/env_var.h"
#include "tensorflow/stream_executor/gpu/gpu_driver.h"

namespace xla {
namespace gpu {
namespace {

// A helper function used to set the size of the executable graph LRU cache
int64 GpuExecGraphCacheSize() {
  int64 cache_size = 0;
  TF_CHECK_OK(
      tensorflow::ReadInt64FromEnvVar("TF_XLA_GPU_EXEC_GRAPH_CACHE_SIZE",
                                      /*default_val=*/100, &cache_size));

  return cache_size;
}
}  // namespace

class MutexedGraphExecCache {
 public:
  // Pushing in a new pair of key and exec graph. Returns flag to specify
  // whether the max cache size has been reached.
  bool UpdateCache(BufferAllocations::KeyType key, void* gpu_exec_graph);

  void* GetExecGraph(BufferAllocations::KeyType key);

  void SetCacheSize(int64 cache_size);

  size_t GetCurrentCacheSize();

  void Initialize(stream_executor::gpu::GpuContext* gpu_context);

  std::list<void*>& GetGpuExecGraphs() { return gpu_exec_graphs_; }

 private:
  tensorflow::mutex exec_graph_cache_mu_;
  std::atomic<int64> cache_size_{0};
  stream_executor::gpu::GpuContext* GUARDED_BY(exec_graph_cache_mu)
      gpu_context_ = nullptr;
  std::list<void*> gpu_exec_graphs_ GUARDED_BY(exec_graph_cache_mu);
  std::unordered_map<BufferAllocations::KeyType, std::list<void*>::iterator>
      gpu_key_to_exec_graphs_map_ GUARDED_BY(exec_graph_cache_mu);
  std::atomic<bool> is_initialized_{false};

  void SetGpuContext(stream_executor::gpu::GpuContext* gpu_context);
};

struct GraphCacheStats {
  std::atomic<uint64> cache_hits{0};
  std::atomic<uint64> temp_buffer_cache_hits{0};
  std::atomic<uint64> cache_miss{0};
  std::atomic<uint64> times_called{0};
  std::atomic<size_t> last_buf_key_hash{0};
  std::atomic<uint64> last_buf_key_hits{0};

  int get_cache_hit_rate() {
    return (cache_hits.load() * 100) / times_called.load();
  }
};
}  // namespace gpu
}  // namespace xla

#endif  // TENSORFLOW_COMPILER_XLA_SERVICE_GPU_GPU_GRAPH_UTIL_H_