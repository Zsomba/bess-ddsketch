#include "ddsketch.h"

#include <iterator>
#include <algorithm>
#include "../../core/utils/common.h"
#include "../../core/utils/ether.h"
#include "../../core/utils/ip.h"
#include "../../core/utils/time.h"
#include "../../core/utils/udp.h"
#include "../../core/modules/timestamp.h"
#include <cmath>

using bess::utils::Ethernet;
using bess::utils::Ipv4;
using bess::utils::Udp;

const Commands DDSketch::cmds = {
    {"empty", "DDSketchCommandEmptyArg", MODULE_CMD_FUNC(&DDSketch::CommandEmpty), Command::THREAD_SAFE},
    {"get_stat", "DDSketchCommandGetStatArg", MODULE_CMD_FUNC(&DDSketch::CommandGetStat), Command::THREAD_SAFE},
    {"get_content", "DDSketchCommandGetContentArg", MODULE_CMD_FUNC(&DDSketch::CommandGetContent), Command::THREAD_SAFE},
    {"get_quantile", "DDSketchCommandGetQuantileArg", MODULE_CMD_FUNC(&DDSketch::CommandGetQuantile), Command::THREAD_SAFE}
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

// Calculate the logarithm with given number and base.
inline int logn(int number, double base) {
    if (number < 0 || base == 1 || base == 0) {
        return -1.0;
    }

    double power = log(number) / log(base);

    return int(power);
}

// Checks whether the bucket1's index is smaller then bucket2
static bool compareBucket(DDSketch::Bucket bucket1, DDSketch::Bucket bucket2){
    return (bucket1.getIndex() < bucket2.getIndex());
}

// Insert the given value into its corresponding bucket
void DDSketch::insertValue(long value) {
    value = abs(value);

    uint64_t index = ceil(logn(value, lambda));

    if (buckets.empty()) {
        addBucket(index);

        return;
    }

    std::vector<DDSketch::Bucket>::iterator bucket = getBucket(index);

    if (bucket != buckets.end()) {
        bucket->increaseCounter();

        return;
    } 
    else if (buckets.size() >= max_bucket_number) {
        std::vector<DDSketch::Bucket>::iterator index_to_delete = getSmallestBucket();
        overflowBucket(index_to_delete, index);

        return;
    } 
    else {
        addBucket(index);

        return;
    }

}

// Clears the collected data.
CommandResponse DDSketch::CommandEmpty(const ddsketch::pb::DDSketchCommandEmptyArg &){
    DDSketch::buckets.clear();

    return CommandResponse();
}

// Returns the status of the collected data.
CommandResponse DDSketch::CommandGetStat(const ddsketch::pb::DDSketchCommandGetStatArg &){
    ddsketch::pb::DDSketchCommandGetStatResponse response;

    response.set_packet_number(DDSketch::getPacketNumber());
    response.set_bucket_number(buckets.size());
    response.set_max_bucket_number(DDSketch::max_bucket_number);
    response.set_accuracy(DDSketch::accuracy);
    response.set_lambda(DDSketch::lambda);

    return CommandSuccess(response);
}

// Counts the packets collected.
uint DDSketch::getPacketNumber(){
    uint count = 0;

    for (std::vector<DDSketch::Bucket>::iterator i = DDSketch::buckets.begin(); i != DDSketch::buckets.end(); ++i) {
        count += i->getCounter();
    }

    return count;
}

// Return the value of Counter in the Bucket. This corresponds to
int DDSketch::Bucket::getCounter() {
    return counter;
}

// Returns the status of the collected data.
CommandResponse DDSketch::CommandGetContent(const ddsketch::pb::DDSketchCommandGetContentArg &){
    ddsketch::pb::DDSketchCommandGetContentResponse response;

    // Sort the buckets in assending order
    std::sort(buckets.begin(), buckets.end(), compareBucket);

    for( std::vector<DDSketch::Bucket>::iterator i = buckets.begin(); i != buckets.end(); ++i){
        ddsketch::pb::DDSketchCommandGetContentResponse::Bucket* bucket = response.add_content();
        bucket->set_index(i->index);
        bucket->set_counter(i->counter);
    }    

    return CommandSuccess(response);
}

// Increase the counter of the bucket by the given amount
void DDSketch::Bucket::increaseCounter(int amount){
    counter += amount;

    return;
}

// Returns the index of the bucket
uint64_t DDSketch::Bucket::getIndex(){
    return index;
}

// Returns whether the bucket is empty
bool DDSketch::Bucket::isEmpty(){
    if(counter == 0){
        return true;
    }

    return false;
}

//Adds a bucket with the given bucket index to the vector and returns its iterator.
std::vector<DDSketch::Bucket>::iterator DDSketch::addBucket(uint64_t index, int counter_value) {
    DDSketch::Bucket* new_bucket = new DDSketch::Bucket(index, counter_value);
    DDSketch::buckets.push_back(*new_bucket);

    std::vector<DDSketch::Bucket>::iterator iter = buckets.end();

    for(std::vector<DDSketch::Bucket>::iterator i = buckets.begin(); i != buckets.end(); ++i)
    {
        if(*i == *new_bucket)
        {
            iter = i;
            break;
        }
    }

    return iter;
}

// Creates a new bucket with the value of a soon to be deleted one
std::vector<DDSketch::Bucket>::iterator DDSketch::overflowBucket(std::vector<DDSketch::Bucket>::iterator to_delete, uint64_t new_index) {
    int starting_value;

    if (to_delete == buckets.end()) {
        starting_value = 0;
    } 
    else {
        starting_value = to_delete->getCounter();
        buckets.erase(to_delete);
    }

    std::vector<DDSketch::Bucket>::iterator new_bucket = DDSketch::addBucket(new_index, starting_value);

    new_bucket->increaseCounter();

    return new_bucket;
}

// Returns the iterator of the bucket with the given bucket index, the end of the list if not found.
std::vector<DDSketch::Bucket>::iterator DDSketch::getBucket (uint64_t index) {
    for (std::vector<DDSketch::Bucket>::iterator i = buckets.begin(); i != buckets.end(); ++i) {
        if (i->getIndex() == index) {
            return i;
        }
    }

    return buckets.end();
}

void DDSketch::ProcessBatch(Context *ctx, bess::PacketBatch *batch){
    uint64_t now_ns = tsc_to_ns(rdtsc());
    size_t offset = offset_;

    mcslock_node_t mynode;
    mcs_lock(&lock_, &mynode);

    int package_count = batch->cnt();

    for (int i = 0; i < package_count; ++i){
        uint64_t package_time_ns = 0;
        uint64_t diff = 1;

        if (IsTimestamped(batch->pkts()[i], offset, &package_time_ns)){
            if (now_ns > package_time_ns){
                diff = now_ns - package_time_ns;
            }
            else{
                continue;
            }
        }
        insertValue((long)diff);
    }

    mcs_unlock(&lock_, &mynode);

    RunNextModule(ctx, batch);
}

CommandResponse DDSketch::Init(const ddsketch::pb::DDSketchArg &arg){
    accuracy = arg.accuracy() ? arg.accuracy() : 0.5;

    lambda = (1 + accuracy) / (1 - accuracy);

    max_bucket_number = arg.max_bucket_number() ? arg.max_bucket_number() : 100;

    offset_ = sizeof(Ethernet) + sizeof(Ipv4) + sizeof(Udp);

    mcs_lock_init(&lock_);

    return CommandSuccess();
}

std::vector<DDSketch::Bucket>::iterator DDSketch::getSmallestBucket(){
    std::vector<DDSketch::Bucket>::iterator iter = buckets.begin();
    std::vector<DDSketch::Bucket>::iterator smallest = iter;

    while(iter != buckets.end()){
        if (smallest->getCounter() > iter->getCounter()){
            smallest = iter;
        }

        ++iter;
    }

    return smallest;
}

// Returns the wanted quantile.
CommandResponse DDSketch::CommandGetQuantile(const ddsketch::pb::DDSketchCommandGetQuantileArg &arg){
    ddsketch::pb::DDSketchCommandGetQuantileResponse response;
    uint32_t quantile = arg.quantile() ? (arg.quantile() % 101) : 50;

    uint32_t limit = (getPacketNumber() * quantile) / 100;
    uint32_t count = 0;

    std::sort(buckets.begin(), buckets.end(), compareBucket);

    int index = buckets.begin()->index;

    for (std::vector<Bucket>::iterator i = buckets.begin(); (count <= limit) && (i != buckets.end()); ++i){
        count += i->getCounter();
        index = i->getIndex();
    }

    response.set_quantile(2 * pow(lambda, index) / (lambda+1));

    return CommandSuccess(response);
}

ADD_MODULE(DDSketch, "ddsketch", "Measures package delay with DDSKetch algorithm")

