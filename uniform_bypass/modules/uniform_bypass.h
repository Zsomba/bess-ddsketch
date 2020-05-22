#ifndef BESS_MODULE_UNIFORM_BYPASS_H
#define BESS_MODULE_UNIFORM_BYPASS_H

#include "../../core/module.h"
#include "../../core/pb/uniform_bypass_msg.pb.h"

class UniformBypass final : public Module {
  public:
  static const gate_idx_t kNumIGates = MAX_GATES;
  static const gate_idx_t kNumOGates = MAX_GATES;

  UniformBypass() { 
      max_allowed_workers_ = Worker::kMaxWorkers;
    }

  CommandResponse Init(const uniform_bypass::pb::UniformBypassArg &arg);

  void ProcessBatch(Context *ctx, bess::PacketBatch *batch) override;

 private:
  uint32_t min;
  uint32_t max;

};

#endif