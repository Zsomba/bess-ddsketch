#ifndef BESS_MODULES_DDKSKETCH_H_
#define BESS_MODULES_DDKSKETCH_H_

#include "bess/core/module.h"
#include "bess/core/pb/module_msg.pb.h"
#include "bess/core/utils/histogram.h"
#include "bess/core/utils/mcslock.h"
#include "bess/core/utils/random.h"
#include <vector>

/**
    Realises the DDSketch measuring algorythm.
*/
class DDSketch
{
    class Bucket
    {
        /**
            Constructs the bucket.

            @param index the power of alpha which is the beginning of the interval contained
            in the bucket
            @param counter the number of measured values contained in the bucket when created,
            default is 0
        */
        Bucket(int index, int counter = 0);

        int index;
        int counter;

    public:

        /**
            Increases the counter of the bucket by the given amount.

            @param amount the number to increase the counter with
        */
        void increaseCounter(int amount);

        /**
            Returns the index of the bucket.

            @return the value of index of the bucket
        */
        int getIndex();

        /**
            Return whether the bucket is empty.

            @return true is the bucket is empty, false otherwise
        */
        bool isEmpty();

        /**
            Return the number of values in the bucket

            @return the value of counter
        */
        int getCounter();

    }
    std::vector<Bucket> buckets;
    int max_bucket_number;
    double accuracy;

    /**
        Adds a new bucket to the buckets vector and returns an iterator to it.

        @param index the index of the new bucket
        @return reference to the new bucket
    */
    Bucket& addBucket(int index);

    /**
        Creates a new bucket with the counter of a soon to be deleted bucket

        @param index the index of the bucket to be deleted
        @param new_index the index of the new bucket
        @return reference to the new bucket
    */
    Bucket& overflowBucket(int to_delete_index, int new_index);

public:

    /**
        Constructs a DDSketch measuring object.

        @param accuracy the accuracy of the measurement, default is 0.1
        @param max_buckets the maximum number of buckets in the vector, default is 100
    */
    DDSketch(double accuracy = 0.1, int max_bucket_number = 100)

    /**
        Returns the bucket in the vector, with the corresponding bucket index.

        @param index the bucket index
        @return the bucket with the given bucket index, or NULL if the vector does
        not contain a bucket with the given bucket index
    */
    Bucket& getBucket (int index);

    /**
        Returns the iterator of the bucket with the corresponding bucket index.

        @param index the bucket index
        @return the iterator of the bucket with the given bucket index, or NULL if the vector does
        not contain a bucket with the given bucket index
    */
    Bucket& getBucketIterator (int index);

    /**
        Insert the given value into its corresponding bucket.

        @param value the value to be inserted
    */
    void insertValue(int value);

}

#endif
