#ifndef MERGE_AND_SHRINK_DISTANCES_H
#define MERGE_AND_SHRINK_DISTANCES_H

#include "types.h"

#include <cassert>
#include <iostream>
#include <vector>
#include <limits>

/*
  TODO: Possible interface improvements for this class:
  - Check TODOs in implementation file.

  (Many of these would need performance tests, as distance computation
  can be one of the bottlenecks in our code.)
*/

namespace merge_and_shrink {
struct bnode {
    int src;
    int cost;
    int secondary_cost;
};

class TransitionSystem;
class Distances {
    static const int DISTANCE_UNKNOWN = -1;
    const TransitionSystem &transition_system;

    bool init_distances_computed;
    bool goal_distances_computed;

    void clear_distances();
    int get_num_states() const;
    bool is_unit_cost() const;

    void compute_init_distances_general_cost();
    void compute_goal_distances_general_cost();



    std::vector<std::vector<std::pair<int, int>>> final_entry_backward_graph;
    std::vector<std::vector<int>> init_distances_bounded_cost;


    mutable std::vector<std::vector<std::pair<int,int>>> per_bound_goal_distances;
    mutable std::vector<std::vector<std::pair<int,int>>> per_bound_init_distances;

    std::vector<std::vector<std::pair<int, std::pair<int,int>>>> reverse_transition_graph;
    int global_cost_bound = std::numeric_limits<int>::max();
    

public:
    explicit Distances(const TransitionSystem &transition_system, int global_cost_bound);
    ~Distances() = default;

    bool are_init_distances_computed() const {
        return init_distances_computed;
    }

    bool are_goal_distances_computed() const {
        return goal_distances_computed;
    }

    /* Currently, computing from goal states for all states */
    void recompute_goal_distances(int from_state, int cost_bound);
    void compute_initial_distances_bounded_cost(int cost_bound);

    void compute_distances(
        bool compute_init_distances,
        bool compute_goal_distances,
        Verbosity verbosity);

    /*
      Update distances according to the given abstraction. If the abstraction
      is not f-preserving, distances are directly recomputed.

      It is OK for the abstraction to drop states, but then all
      dropped states must be unreachable or irrelevant. (Otherwise,
      the method might fail to detect that the distance information is
      out of date.)
    */
    void apply_abstraction(
        const StateEquivalenceRelation &state_equivalence_relation,
        bool compute_init_distances,
        bool compute_goal_distances,
        Verbosity verbosity);

    int get_init_distance(int state, int cost_bound = std::numeric_limits<int>::max()) const;
    int get_goal_distance(int state, int cost_bound = std::numeric_limits<int>::max()) const;

    void dump() const;
    void statistics() const;
};
}

#endif
