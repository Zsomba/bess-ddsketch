#include "ddsketch.h"

#include <iterator>

#include "../utils/common.h"
#include "../utils/ether.h"
#include "../utils/ip.h"
#include "../utils/time.h"
#include "../utils/udp.h"
#include "timestamp.h"
#include <cmath>

using bess::utils::Ethernet;
using bess::utils::Ipv4;
using bess::utils::Udp;

const Commands DDSketch::cmds = {
    {"empty", "DDSketchCommandEmptyArg", MODULE_CMD_FUNC(&DDSketch::CommandEmpty), Command::THREAD_SAFE},
    {"get_stat", "DDSketchCommandGetStatArg", MODULE_CMD_FUNC(&DDSketch::CommandGetStat), Command::THREAD_SAFE},
    {"get_content", "DDSketchCommandGetContentArg", MODULE_CMD_FUNC(&DDSketch::CommandGetContent), Command::THREAD_SAFE}
};

static bool IsTimestamped(bess::Packet *pkt, size_t offset, uint64_t *time) {
    auto *marker = pkt->head_data<Timestamp::MarkerType *>(offset);

    if (*marker == Timestamp::kMarker) {
        *time = *reinterpret_cast<uint64_t *>(marker + 1);
        return true;
    }
    return false;
}

// Calculate the logarithm with given number and base.
int logn(int number, double base) {
    if (number < 0 || base == 1 || base == 0) {
        return -1.0;                                // exception?
    }

    double power = log(number) / log(base);

    return int(power);
}

// Insert the given value into its corresponding bucket
void DDSketch::insertValue(int value) {
    value = abs(value);

    long index = ceil(logn(value, lambda));

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
CommandResponse DDSketch::CommandEmpty(const bess::pb::DDSketchCommandEmptyArg &){
    DDSketch::buckets.clear();

    return CommandResponse();
}

// Returns the status of the collected data.
CommandResponse DDSketch::CommandGetStat(const bess::pb::DDSketchCommandGetStatArg &){
    bess::pb::DDSketchCommandGetStatResponse response;

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
CommandResponse DDSketch::CommandGetContent(const bess::pb::DDSketchCommandGetContentArg &){
    bess::pb::DDSketchCommandGetContentResponse response;

    // std::vector<bess::pb::DDSketchCommandGetContentResponse::Bucket> sent_buckets;
    

    for( std::vector<DDSketch::Bucket>::iterator i = buckets.begin(); i != buckets.end(); ++i){
        bess::pb::DDSketchCommandGetContentResponse::Bucket* bucket = response.add_content();
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
long DDSketch::Bucket::getIndex(){
    return index;
}

// Returns whether the bucket is empty
bool DDSketch::Bucket::isEmpty(){
    if(counter == 0){
        return true;
    }

    return false;
}

//Makes Bucket comparison by their index
/*bool operator==(const DDSketch::Bucket bucket1, const DDSketch::Bucket bucket2)
{
    return bucket1.index == bucket2.index;
}*/

//Adds a bucket with the given bucket index to the vector and returns its iterator.
std::vector<DDSketch::Bucket>::iterator DDSketch::addBucket(long index, int counter_value) {
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
std::vector<DDSketch::Bucket>::iterator DDSketch::overflowBucket(std::vector<DDSketch::Bucket>::iterator to_delete, long new_index) {
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
std::vector<DDSketch::Bucket>::iterator DDSketch::getBucket (long index) {
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
        long diff = 1;

        if (IsTimestamped(batch->pkts()[i], offset, &package_time_ns)){
            if (now_ns > package_time_ns){
                diff = long(now_ns - package_time_ns);
            }
            else{
                continue;
            }
        }
        insertValue(diff);
    }

    mcs_unlock(&lock_, &mynode);

    RunNextModule(ctx, batch);
}

CommandResponse DDSketch::Init(const bess::pb::DDSketchArg &arg){
    double accuracy = arg.accuracy();
    uint max_bucket_number = arg.max_bucket_number();

    if (accuracy == 0){
        accuracy = 0.1;
    }
    if (max_bucket_number == 0){
        max_bucket_number = 100;
    }

    if (arg.offset()) {
        offset_ = arg.offset();
    } 
    else {
         offset_ = sizeof(Ethernet) + sizeof(Ipv4) + sizeof(Udp);
    }

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

ADD_MODULE(DDSketch, "ddsketch", "Measures package jitter with DDSKetch algorithm")

