#ifndef BESS_MODULE_GAUSSIAN_BYPASS_H
#define BESS_MODULE_GAUSSIAN_BYPASS_H

#include "../../core/module.h"
#include "../../core/pb/gaussian_bypass_msg.pb.h"

class GaussianBypass final : public Module {
  public:
  static const gate_idx_t kNumIGates = MAX_GATES;
  static const gate_idx_t kNumOGates = MAX_GATES;

  GaussianBypass() { 
      max_allowed_workers_ = Worker::kMaxWorkers;
    }

  CommandResponse Init(const gaussian_bypass::pb::GaussianBypassArg &arg);

  void ProcessBatch(Context *ctx, bess::PacketBatch *batch) override;

 private:
  uint32_t mean;
  uint32_t deveation;

};

#endif