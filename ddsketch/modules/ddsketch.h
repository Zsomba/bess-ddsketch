#ifndef BESS_MODULES_DDKSKETCH_H_
#define BESS_MODULES_DDKSKETCH_H_

#include "../../core/module.h"
#include "../../core/pb/ddsketch_msg.pb.h"
#include "../../core/utils/mcslock.h"
#include "../../core/utils/random.h"
#include <vector>

/**
 * Realises the DDSketch measuring algorythm.
 */
class DDSketch final: public Module {
    public:

    static const Commands cmds;

    struct Bucket {
        uint64_t index;
        int counter;

        /**
         * Constructs the bucket.
         * @param index the power of alpha which is the beginning of the interval contained
         * in the bucket
         * @param counter the number of measured values contained in the bucket when created,
         * default is 0
         */
        Bucket(uint64_t index, int counter = 0): index(index), counter(counter){};

        /**
         * Increases the counter of the bucket by the given amount.
         * @param amount the number to increase the counter with
         */
        void increaseCounter(int amount = 1);

        /**
         * Returns the index of the bucket.
         * @return the value of index of the bucket
         */
        uint64_t getIndex();

        /**
         * Return whether the bucket is empty.
         * @return true is the bucket is empty, false otherwise
         */
        bool isEmpty();

        /**
         * Return the number of values in the bucket
         * @return the value of counter
         */
        int getCounter();

        /**
         * Makes Bucket comparison by their index
         * @param bucket1 the bucket to compare
         * @return true if the two buckets' indexes are equal, false otherwise
         */;
        inline bool operator==(const Bucket bucket){ return this->index == bucket.index; }

    };

    private:

    std::vector<Bucket> buckets;
    double accuracy;
    uint max_bucket_number;
    double lambda;

    /**
        Adds a new bucket to the buckets vector and returns an iterator to it.

        @param index the index of the new bucket
        @return reference to the new bucket
    */
    std::vector<Bucket>::iterator addBucket(uint64_t index, int counter_value = 0);

    /**
        Creates a new bucket with the counter of a soon to be deleted bucket

        @param index the index of the bucket to be deleted
        @param new_index the index of the new bucket
        @return iterator to the new bucket
    */
    std::vector<Bucket>::iterator overflowBucket(std::vector<Bucket>::iterator to_delete, uint64_t new_index);

    std::vector<Bucket>::iterator getSmallestBucket();

    /**
     * Counts the packets collected.
     */
    uint getPacketNumber();

    public:

    mcslock lock_;
    size_t offset_;

    /**
        Constructs a DDSketch measuring object.

        @param accuracy the accuracy of the measurement, default is 0.1
        @param max_buckets the maximum number of buckets in the vector, default is 100
    */
    DDSketch()
        :Module(),
         accuracy(),
         max_bucket_number(){
             max_allowed_workers_ = Worker::kMaxWorkers;
         }

    /**
     * Clears the collected data.
     */
    CommandResponse CommandEmpty(const ddsketch::pb::DDSketchCommandEmptyArg &arg);

    /**
     * Returns the status of the collected adta.
     */
    CommandResponse CommandGetStat(const ddsketch::pb::DDSketchCommandGetStatArg &arg);

    /**
     * Returns the status of the collected adta.
     */
    CommandResponse CommandGetContent(const ddsketch::pb::DDSketchCommandGetContentArg &arg);

    /**
     * Returns the wanted quantile.
     */
    CommandResponse CommandGetQuantile(const ddsketch::pb::DDSketchCommandGetQuantileArg &arg);

    /**
        Returns the iterator of the bucket with the corresponding bucket index.

        @param index the bucket index
        @return the iterator of the bucket with the given bucket index, or NULL if the vector does
        not contain a bucket with the given bucket index
    */
    std::vector<Bucket>::iterator getBucket (uint64_t index);

    /**
        Insert the given value into its corresponding bucket.

        @param value the value to be inserted
    */
    void insertValue(long value);

    void ProcessBatch(Context *ctx, bess::PacketBatch *batch);
    CommandResponse Init(const ddsketch::pb::DDSketchArg &arg);

};

#endif
