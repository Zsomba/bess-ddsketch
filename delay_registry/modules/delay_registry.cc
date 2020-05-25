#include "delay_registry.h"
#include "../../core/utils/common.h"
#include "../../core/utils/ether.h"
#include "../../core/utils/ip.h"
#include "../../core/utils/time.h"
#include "../../core/utils/udp.h"
#include "../../core/modules/timestamp.h"
#include <iterator>
#include <algorithm>

using bess::utils::Ethernet;
using bess::utils::Ipv4;
using bess::utils::Udp;

const Commands DelayRegistry::cmds = {
    {"get_content", "DelayRegistryCommandGetContentArg", MODULE_CMD_FUNC(&DelayRegistry::CommandGetContent), Command::THREAD_SAFE}
};

// Checks whether the packet was timestamped
static bool IsTimestamped(bess::Packet *pkt, size_t offset, uint64_t *time) {
    auto *marker = pkt->head_data<Timestamp::MarkerType *>(offset);

    if (*marker == Timestamp::kMarker) {
        *time = *reinterpret_cast<uint64_t *>(marker + 1);
        return true;
    }
    return false;
}

// 

// Get the content of the registry.
CommandResponse DelayRegistry::CommandGetContent(const delay_registry::pb::DelayRegistryCommandGetContentArg &){
    delay_registry::pb::DelayRegistryCommandGetContentResponse response;

    for (std::vector<Bucket>::iterator i = registry.begin(); i != registry.end(); ++i){
        delay_registry::pb::DelayRegistryCommandGetContentResponse::Bucket* content = response.add_content();

        content->set_value(i->value);
        content->set_count(i->count);
    }

    return CommandSuccess(response);
}

CommandResponse DelayRegistry::Init(const delay_registry::pb::DelayRegistryArg &){
    offset_ = sizeof(Ethernet) + sizeof(Ipv4) + sizeof(Udp);

    return CommandSuccess();
}

// Inserts the prosessed value into the vector
inline void DelayRegistry::insertValue(uint64_t value){
    for (std::vector<Bucket>::iterator i = registry.begin(); i != registry.end(); ++i){
        if (i->value == value){
            ++ i->count;
            return;
        }
    }

    registry.push_back(Bucket(value));

    return;
}

bool DelayRegistry::Bucket::operator== (Bucket b){
    return value == b.value;
}

void DelayRegistry::ProcessBatch(Context *ctx, bess::PacketBatch *batch){
    uint64_t now_tsc = rdtsc();
    uint32_t cnt = batch->cnt();

    uint64_t arrive_tsc = 0;

    for (uint32_t i = 0; i < cnt; ++i){
        if (IsTimestamped(batch->pkts()[i], offset_, &arrive_tsc)){
            if (now_tsc > arrive_tsc)
            {
                uint64_t delay = now_tsc - arrive_tsc;
                insertValue(delay / (uint64_t)1000000000);
            }
        }
    }

    RunNextModule(ctx, batch);
}

ADD_MODULE(DelayRegistry, "delay_registry", "Registers the delay of the processed packets.")