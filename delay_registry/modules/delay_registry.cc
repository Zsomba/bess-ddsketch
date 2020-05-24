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

    for (std::vector<uint64_t>::iterator i = registry.begin(); i != registry.end(); ++i){
        response.add_content(*i);
    }

    return CommandSuccess(response);
}

CommandResponse DelayRegistry::Init(const delay_registry::pb::DelayRegistryArg &){
    offset_ = sizeof(Ethernet) + sizeof(Ipv4) + sizeof(Udp);

    return CommandSuccess();
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
                std::vector<uint64_t>::iterator position = std::find(registry.begin(), registry.end(), delay);
                if (position == registry.end()){
                    registry.push_back(delay);
                }
            }
        }
    }

    RunNextModule(ctx, batch);
}

ADD_MODULE(DelayRegistry, "delay_registry", "Registers the delay of the processed packets.")