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

    class Bucket{
        public:

        uint64_t value;
        uint64_t count;

        Bucket(uint64_t value)
        :value(value),
        count(0){}

        bool operator== (Bucket b);
    };

    static const Commands cmds;
    std::vector<Bucket> registry;

    DelayRegistry()
        :Module(){
            max_allowed_workers_ = Worker::kMaxWorkers;
        }

    void ProcessBatch(Context *ctx, bess::PacketBatch *batch);

    CommandResponse Init(const delay_registry::pb::DelayRegistryArg &);

    CommandResponse CommandGetContent(const delay_registry::pb::DelayRegistryCommandGetContentArg &);

    private:

    /**
     * Inserts the processed value into the vector.
     */
    inline void insertValue(uint64_t value);

    size_t offset_;
};

#endif