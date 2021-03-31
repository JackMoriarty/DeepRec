#ifndef TENSORFLOW_SERVING_MODEL_SESSION_H
#define TENSORFLOW_SERVING_MODEL_SESSION_H

#include "odl_processor/serving/model_config.h"
#include "odl_processor/framework/model_version.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/protobuf/meta_graph.pb.h"
#include "tensorflow/core/framework/tensor.h"
#include <thread>
#include <atomic>

namespace tensorflow {
class SessionOptions;
class RunOptions;
class Session;
class Tensor;

namespace processor {
class IFeatureStoreMgr;
class Request;
class Response;
struct ModelSession {
  ModelSession(Session* s, const Version& version,
      IFeatureStoreMgr* sparse_storage);
  ModelSession(Session* s, const Version& version);
  virtual ~ModelSession();

  Status Predict(Request& req, Response& resp);
  Status LocalPredict(Request& req, Response& resp);

  Session* session_ = nullptr;
  //IFeatureStoreMgr* sparse_storage_ = nullptr;
  
  std::string sparse_storage_name_;
  Tensor sparse_storage_tensor_;
  std::string model_version_name_;
  Tensor model_version_tensor_;
  std::atomic<int64> counter_;
  // Local storage or remote storage for sparse variable.
  bool is_local_ = true;
};

class ModelSessionMgr {
 public:
  ModelSessionMgr(const MetaGraphDef& meta_graph_def,
      SessionOptions* session_options, RunOptions* run_options);
  virtual ~ModelSessionMgr();

  Status Predict(Request& req, Response& resp);
  Status LocalPredict(Request& req, Response& resp);

  Status CreateModelSession(
      const Version& version, const char* ckpt_name,
      IFeatureStoreMgr* sparse_storage,
      bool is_incr_ckpt, bool is_initialize,
      ModelConfig* config);

  Status CreateModelSession(
      const Version& version, const char* ckpt_name,
      bool is_incr_ckpt, ModelConfig* config);
 
  Status CleanupModelSession();

 private:
  virtual Status CreateSession(Session** sess);
  virtual Status RunRestoreOps(
      const char* ckpt_name, int64 full_ckpt_version,
      const char* savedmodel_dir, Session* session,
      IFeatureStoreMgr* sparse_storage,
      bool is_incr_ckpt, bool update_sparse,
      int64_t latest_version);
  
  void ResetServingSession(Session* session, const Version& version,
      IFeatureStoreMgr* sparse_storage);

  void ClearLoop();

 protected:
  ModelSession* serving_session_ = nullptr;

  MetaGraphDef meta_graph_def_;
  SessionOptions* session_options_;
  RunOptions* run_options_;
  std::vector<AssetFileDef> asset_file_defs_;

  std::thread* clear_session_thread_ = nullptr;
  std::vector<ModelSession*> sessions_;
  mutex mu_;
  volatile bool is_stop_ = false;
};

} // processor
} // tensorflow

#endif // TENSORFLOW_SERVING_MODEL_SESSION_H

