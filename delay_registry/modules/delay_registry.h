#ifndef BESS_MODULES_DELAYREGISTRY_H_
#define BESS_MODULES_DELAYREGISTRY_H_

#include "../../core/module.h"
#include "../../core/pb/delay_registry_msg.pb.h"
#include <vector>

/**
 * Registers the delay of the processed packets.
 */
class DelayRegistry final: public Module{
    public:

    static const Commands cmds;

    DelayRegistry()
        :Module(){
            max_allowed_workers_ = Worker::kMaxWorkers;
        }

    void ProcessBatch(Context *ctx, bess::PacketBatch *batch);

    CommandResponse Init(const delay_registry::pb::DelayRegistryArg &);

    CommandResponse CommandGetContent(const delay_registry::pb::DelayRegistryCommandGetContentArg &);

    private:

    std::vector<uint64_t> registry;
    size_t offset_;
};

#endif