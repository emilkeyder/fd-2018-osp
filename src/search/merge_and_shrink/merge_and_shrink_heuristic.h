#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "../heuristic.h"

#include <memory>

namespace utils {
class CountdownTimer;
class Timer;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;
class LabelReduction;
class MergeAndShrinkRepresentation;
class Distances;
class MergeStrategyFactory;
class ShrinkStrategy;
class TransitionSystem;
enum class Verbosity;

class MergeAndShrinkHeuristic : public Heuristic {
    // TODO: when the option parser supports it, the following should become
    // unique pointers.
    std::shared_ptr<MergeStrategyFactory> merge_strategy_factory;
    std::shared_ptr<ShrinkStrategy> shrink_strategy;
    std::shared_ptr<LabelReduction> label_reduction;

    // Options for shrinking
    // Hard limit: the maximum size of a transition system at any point.
    const int max_states;
    // Hard limit: the maximum size of a transition system before being merged.
    const int max_states_before_merge;
    /* A soft limit for triggering shrinking even if the hard limits
       max_states and max_states_before_merge are not violated. */
    const int shrink_threshold_before_merge;

    // Options for pruning
    const bool prune_unreachable_states;
    const bool prune_irrelevant_states;

    const Verbosity verbosity;
    const double main_loop_max_time;

    long starting_peak_memory;
    // The final merge-and-shrink representation, storing goal distances.
    std::unique_ptr<MergeAndShrinkRepresentation> mas_representation;
    std::unique_ptr<Distances> mas_distances;
    std::unique_ptr<TransitionSystem> mas_transition_system;

    void finalize_factor(FactoredTransitionSystem &fts, int index);
    int prune_fts(FactoredTransitionSystem &fts, const utils::Timer &timer) const;
    int main_loop(FactoredTransitionSystem &fts, const utils::Timer &timer);
    void build(const utils::Timer &timer);

    void report_peak_memory_delta(bool final = false) const;
    void dump_options() const;
    void warn_on_unusual_options() const;
    bool ran_out_of_time(const utils::CountdownTimer &timer) const;

    bool use_cost_bound;

protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;
    virtual int compute_heuristic_w_bound(const GlobalState &state, int cost_bound) override;
public:
    virtual void notify_state_transition(const GlobalState &parent_state, OperatorID op_id,
					 const GlobalState &state) override{
      (void) parent_state;
      (void) op_id;
      if (cache_evaluator_values && use_cost_bound) {
        /* TODO:  It may be more efficient to check that the reached landmark
           set has actually changed and only then mark the h value as dirty. */
        heuristic_cache[state].dirty = true;
      }
    }
    
    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) override {
        evals.insert(this);
    }

    explicit MergeAndShrinkHeuristic(const options::Options &opts);
    virtual ~MergeAndShrinkHeuristic() override = default;
    static void add_shrink_limit_options_to_parser(options::OptionParser &parser);
    static void handle_shrink_limit_options_defaults(options::Options &opts);
};
}

#endif
