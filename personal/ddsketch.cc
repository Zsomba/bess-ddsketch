#include "ddsketch.h"

#include <iterator>

#include "bess/core/modules/utils/common.h"
#include "bess/core/modules/utils/ether.h"
#include "bess/core/modules/utils/ip.h"
#include "bess/core/modules/utils/time.h"
#include "bess/core/modules/utils/udp.h"
#include "timestamp.h"
#include <cmath>

using bess::utils::Ethernet;
using bess::utils::Ipv4;
using bess::utils::Udp;

static bool IsTimestamped(bess::Packet *pkt, size_t offset, uint64_t *time)
{
    auto *marker = pkt->head_data<Timestamp::MarkerType *>(offset);

    if (*marker == Timestamp::kMarker)
    {
        *time = *reinterpret_cast<uint64_t *>(marker + 1);
        return true;
    }
    return false;
}

// Calculate the logarithm of given number in given base.
double logn(double number, int base)
{
    if (number < 0 || base == 1 || base == 0)
    {
        return -1.0;
    }

    double power = log(number) / log(base);

    return power;
}

// Insert the given value into its corresponding bucket
void DDSketch::insertValue(int value)
{
    value = abs(value);

    int index = math::ceil(logn(base, value));

    if (buckets.empty())
    {
        addBucket(index);

        return;
    }

    Bucket bucket = getBucket(index);

    if (bucket != NULL)
    {
        bucket.increaseCounter();

        return;
    }
    else if (buckets.size != max_bucket_number)
    {
        vector<Bucket>.iterator index_to_delete = getSmallestBucket();
        overflowBucket(index_to_delete, index);

        return;
    }
    else
    {
        addBucket(index);

        return;
    }

}

// Return the value of Counter in the Bucket. This corresponds to
int DDSketch::Bucket::getCounter()
{
    return counter;
}

//Adds a bucket with the given bucket index to the vector and returns its iterator.
Bucket& DDSketch::addBucket(int index, int counter_value = 0)
{
    Bucket new_bucket = Bucket(i, counter_value);
    buckets.push_back(new_bucket);

    return buckets(buckets.rbegin())
}

// Creates a new bucket with the value of a soon to be deleted one
Bucket& DDSketch::overflowBucket(int to_delete_index, int new_index)
{
    int starting_value;

    Bucket to_delete = getBucket(to_delete_index);
    if (to_delete == NULL)
    {
        starting_value = 0;
    }
    else
    {
        starting_value = to_delete.getCounter();
        buckets.erase(getBucketIterator(to_delete_index));
    }

    Bucket new_bucket = addBucket(new_index, starting_value);

    new_bucket.increaseCounter();

    return new_bucket;
}

//Returns the bucket with the given bucket index, NULL if not found.
Bucket& DDSketch::getBucket (int index)
{
    for(std::vector<Bucket>::iterator i = buckets.begin(); i != buckets.end(); ++i)
    {
        if (i.getIndex() == index)
        {
            return buckets[i];
        }
    }

    return NULL;
}

//Returns the iterator of the bucket with the given bucket index, NULL if not found.
Bucket& DDSketch::getBucketIterator (int index)
{
    for(std::vector<Bucket>::iterator i = buckets.begin(); i != buckets.end(); ++i)
    {
        if (i.getIndex() == index)
        {
            return i;
        }
    }

    return NULL;
}

// Constructs a DDSketch object
DDSketch::DDSketch(double accuracy = 0.1, int max_bucket_number = 100)
    :Module()
{
    this.accuracy = accuracy;
    this.max_bucket_number = max_bucket_number;
}
