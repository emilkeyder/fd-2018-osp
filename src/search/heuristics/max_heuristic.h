#ifndef HEURISTICS_MAX_HEURISTIC_H
#define HEURISTICS_MAX_HEURISTIC_H

#include "relaxation_heuristic.h"

#include "../algorithms/priority_queues.h"

#include <cassert>
#include <queue>

namespace max_heuristic {
using relaxation_heuristic::Proposition;
using relaxation_heuristic::UnaryOperator;

class HSPMaxHeuristic : public relaxation_heuristic::RelaxationHeuristic {
  // priority_queues::AdaptiveQueue<Proposition *> queue;

  struct PQEntry {
    std::pair<int, int> costs;
    Proposition* prop;

    bool operator> (const PQEntry& other) const {
      if (costs.first > other.costs.first) return true;
      if (costs.first == other.costs.first && costs.second > other.costs.second) return true;
      return false;
    };
    
    PQEntry(const std::pair<int,int>& costs, Proposition* prop) : 
      costs(costs), prop(prop) {}
  };
  
  std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> queue;

    void setup_exploration_queue(int bound);
    void setup_exploration_queue_state(const State &state);
    void relaxed_exploration(int bound);

    void enqueue_if_necessary(Proposition *prop, int cost, int bounded_cost) {
        assert(cost >= 0);
	bool enqueue = false;
        if (prop->cost == -1 || prop->cost > cost) {
            prop->cost = cost;
	    enqueue = true;
        }
	if (prop->bounded_cost == -1 || prop->bounded_cost > bounded_cost) {
	  prop->bounded_cost = bounded_cost;
	  enqueue = true;
	}
	if (enqueue) {
	  queue.push(PQEntry(std::make_pair(prop->cost, prop->bounded_cost), prop));
	}
        assert(prop->cost != -1 && prop->cost <= cost);
    }

    bool use_cost_bound;

protected:
    int compute_heuristic(const GlobalState &global_state, int cost_bound);
    virtual int compute_heuristic(const GlobalState &global_state) override;
    virtual int compute_heuristic_w_bound(
	const GlobalState &global_state, int cost_bound) override;
public:
    
    virtual void notify_state_transition(const GlobalState &parent_state,
                                         OperatorID op_id,
                                         const GlobalState &state) override;

    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) override {
        evals.insert(this);
    }

    HSPMaxHeuristic(const options::Options &opts);
    ~HSPMaxHeuristic();
};
}

#endif
