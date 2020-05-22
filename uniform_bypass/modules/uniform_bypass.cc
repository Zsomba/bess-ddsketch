#include "uniform_bypass.h"
#include <random>
#include <ctime>

CommandResponse UniformBypass::Init(const uniform_bypass::pb::UniformBypassArg &arg){
    min = arg.min() ? arg.min() : 1;
    max = arg.max() ? arg.max() : 60;

    return CommandSuccess();
}

void UniformBypass::ProcessBatch(Context *ctx, bess::PacketBatch *batch){
    uint64_t start_tsc = rdtsc();
    uint32_t cnt = batch->cnt();
    uint32_t delay = 0;

    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::uniform_int_distribution<uint64_t> distribution(min, max);

    for(uint32_t i = 1; i <= cnt; ++i){
        delay += distribution(gen);
    }

    uint64_t stop_tsc = start_tsc + delay;

    while(rdtsc() < stop_tsc){
        _mm_pause();
    }

    RunNextModule(ctx, batch);
}

ADD_MODULE(UniformBypass, "uniform_bypass", "Bypasses packets with uniform deveation random delay.")