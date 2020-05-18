#include "gaussian_bypass.h"
#include <random>
#include <ctime>

CommandResponse GaussianBypass::Init(const gaussian_bypass::pb::GaussianBypassArg &arg){
  GaussianBypass::mean = arg.mean();
  GaussianBypass::deveation = arg.deveation();

  return CommandSuccess();
}

void GaussianBypass::ProcessBatch(Context *ctx, bess::PacketBatch *batch){
    uint64_t start_tsc = rdtsc();
    uint32_t cnt = batch->cnt();
    uint32_t delay = 0;

    //std::mt19937 generator;
    //generator.seed(std::time(0));
    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::normal_distribution<> distribution(GaussianBypass::mean, GaussianBypass::deveation);

    for(uint32_t i = 1; i <= cnt; ++i){
        delay += std::round(distribution(gen));
    }

    //uint64_t stop_tsc = start_tsc + (uint64_t)delay;

    uint64_t stop_tsc = start_tsc + (uint64_t)std::round(distribution(gen));

    while(rdtsc() < stop_tsc){
        _mm_pause();
    }

    RunNextModule(ctx, batch);
}

ADD_MODULE(GaussianBypass, "gaussian_bypass", "Bypasses packets with gaussian deveation random delay.")