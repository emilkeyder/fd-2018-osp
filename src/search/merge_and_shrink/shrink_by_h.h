#ifndef MERGE_AND_SHRINK_SHRINK_BY_H_H
#define MERGE_AND_SHRINK_SHRINK_BY_H_H

#include "shrink_bucket_based.h"

#include <vector>

namespace options {
class Options;
}

namespace merge_and_shrink {
/*
  NOTE: In case where we must merge across buckets (i.e. when
  the number of (f, h) pairs is larger than the number of
  permitted abstract states), this shrink strategy will *not* make
  an effort to be at least be h-preserving.

  This could be improved, but not without complicating the code.
  Usually we set the number of abstract states large enough that we
  do not need to merge across buckets. Therefore the complication
  might not be worth the code maintenance cost.
*/
class ShrinkByH : public ShrinkBucketBased {

protected:
    virtual std::string name() const override;
    virtual void dump_strategy_specific_options() const override;

public:
    explicit ShrinkByH(const options::Options &opts);
    virtual ~ShrinkByH() override = default;

    std::vector<Bucket> partition_into_buckets(
        const TransitionSystem &ts,
        const Distances &distances) const override;
        
    virtual bool requires_init_distances() const override {
        return false;
    }
    virtual bool requires_goal_distances() const override {
        return true;
    }
};
}

#endif
