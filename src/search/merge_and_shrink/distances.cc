#include "distances.h"

#include "label_equivalence_relation.h"
#include "transition_system.h"

#include "../algorithms/priority_queues.h"

#include <cassert>
#include <deque>
#include <queue>

using namespace std;

namespace merge_and_shrink {
const int Distances::DISTANCE_UNKNOWN;

  Distances::Distances(const TransitionSystem &transition_system, int global_cost_bound)
  : transition_system(transition_system), 
    per_bound_goal_distances(transition_system.get_size(), std::vector<std::pair<int,int>>()), 
    global_cost_bound(global_cost_bound) {
  clear_distances();
}

void Distances::clear_distances() {
    init_distances_computed = false;
    goal_distances_computed = false;
    per_bound_goal_distances.clear();
    per_bound_init_distances.clear();
}

int Distances::get_num_states() const {
    return transition_system.get_size();
}

  using TransitionGraph = std::vector<std::vector<std::pair<int, std::pair<int,int>>>>;

TransitionGraph get_transition_graph(const TransitionSystem& ts,
				     bool reverse) {
  TransitionGraph tg(ts.get_size());
  for (const GroupAndTransitions &gat : ts) {
    const LabelGroup &label_group = gat.label_group;
    const vector<Transition> &transitions = gat.transitions;
    for (const Transition &transition : transitions) {
      //      if (transition.src == transition.target) continue;

      int source = reverse ? transition.target : transition.src;
      int target = reverse ? transition.src : transition.target;

      tg[source].push_back({target, {label_group.get_secondary_cost(), label_group.get_cost()}});      
    }
  }
  
  return tg;
}

void compute_per_bound_distances_internal(const TransitionGraph& tg, 
					  const std::vector<int>& starting_states, 
					  std::vector<std::vector<std::pair<int,int> > >& distances, 
					  int cost_bound) {
  struct PQEntry {
    int state;
    std::pair<int,int> costs; // <secondary_cost, primary_cost>
    
    bool operator> (const PQEntry& other) const {
      if (costs.first > other.costs.first) return true;
      if (costs.first == other.costs.first && costs.second > other.costs.second) return true;
      return false;
    };
  };
  
  std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> queue;
  
  for (int state : starting_states) {
    cout << "Init of state " << state << " with costs (0,0)" << endl;
    queue.push({state, {0, 0}});
  }
    
  while (!queue.empty()) {
    PQEntry pq_entry = queue.top();
    queue.pop();
    
    std::vector<std::pair<int, int>>& state_dists = distances[pq_entry.state];

    cout << "Popped state " << pq_entry.state << " with current cost vector ";
    for (const auto& entry : state_dists) {
      cout << "(" << entry.first << ", " << entry.second << "), ";
    }
    cout << endl;
    
    if (state_dists.empty() || 
	// previously discovered at a lower bounded cost with higher primary cost. 
	(state_dists.back().first < pq_entry.costs.first &&
	 state_dists.back().second > pq_entry.costs.second)) {
      state_dists.push_back(pq_entry.costs);
	
      for (const std::pair<int, std::pair<int, int>>& adj_state : tg[pq_entry.state]) {
	// Ignore self loops
	// if (adj_state.first == pq_entry.state) continue;
	if (pq_entry.costs.first + adj_state.second.first > cost_bound) continue;
	
	queue.push({adj_state.first, 
	      {pq_entry.costs.first + adj_state.second.first, 
		  pq_entry.costs.second + adj_state.second.second}});
      }
    }

    cout << "After update state " << pq_entry.state << " has cost vector ";
    for (const auto& entry : state_dists) {
      cout << "(" << entry.first << ", " << entry.second << "), ";
    }
    cout << endl;

  }

  for (size_t i = 0; i < tg.size(); ++i) {
    cout << "Final entry for state " << i << ": ";
    for (const auto& entry : distances[i]) {
      cout << "(" << entry.first << ", " << entry.second << "), ";
    }
    cout << endl;
  }

}		       

void Distances::compute_goal_distances_general_cost() {
  TransitionGraph tg = get_transition_graph(transition_system, true);

  cout << "Transition graph in compute_goal_distances_general_cost()" << endl;
  for (size_t i = 0; i < tg.size(); ++i) {
    for (size_t j = 0; j < tg[i].size(); ++j) {
      cout << i << " --> " << tg[i][j].first 
	   << " (" << tg[i][j].second.first << ", " << tg[i][j].second.second << ")" << endl;
    }
  }

  std::vector<int> goal_states;
  for (int state = 0; state < get_num_states(); ++state) {
    if (transition_system.is_goal_state(state)) {
      goal_states.push_back(state);
    }
  }
  cout << "compute_per_bound_distances_internal() for goal distances" << endl;
  compute_per_bound_distances_internal(tg, goal_states, per_bound_goal_distances, global_cost_bound);
}

void Distances::compute_init_distances_general_cost() {
  TransitionGraph tg = get_transition_graph(transition_system, false);

  cout << "Transition graph in compute_init_distances_general_cost()" << endl;
  for (size_t i = 0; i < tg.size(); ++i) {
    for (size_t j = 0; j < tg[i].size(); ++j) {
      cout << i << " --> " << tg[i][j].first 
	   << " (" << tg[i][j].second.first << ", " << tg[i][j].second.second << ")" << endl;
    }
  }

  std::vector<int> init_states = {transition_system.get_init_state()};

  cout << "compute_per_bound_distances_internal() for init distances" << endl;
  compute_per_bound_distances_internal(tg, init_states, per_bound_init_distances, global_cost_bound);
}

int get_distance(const std::vector<std::pair<int,int>>& state_bounds_and_dists, 
		 int cost_bound) {      
  size_t first_entry_exceeding_bound = 0;
  while (first_entry_exceeding_bound < state_bounds_and_dists.size() &&
	 state_bounds_and_dists[first_entry_exceeding_bound].first <= cost_bound) {
    ++first_entry_exceeding_bound;
  }
  
  if (first_entry_exceeding_bound == 0) return INF;
  
  return state_bounds_and_dists[first_entry_exceeding_bound - 1].second;
}

int Distances::get_goal_distance(int state, int cost_bound) const {
  cout << "get_goal_distance()" << endl;
  const std::vector<std::pair<int,int>>& state_bounds_and_goal_dists = 
    per_bound_goal_distances[state];

  cout << "Bounds and eval dists for state " << state << " : " << flush;
  for (const auto& entry : state_bounds_and_goal_dists) {
    cout << "(" << entry.first << ", " << entry.second << "), " << flush;
  }
  cout << endl;

  return get_distance(state_bounds_and_goal_dists, cost_bound);
}

int Distances::get_init_distance(int state, int cost_bound) const {
  const std::vector<std::pair<int,int>>& state_bounds_and_init_dists = 
    per_bound_init_distances[state];
  return get_distance(state_bounds_and_init_dists, cost_bound);
}

bool Distances::is_unit_cost() const {
    /*
      TODO: Is this a good implementation? It differs from the
      previous implementation in transition_system.cc because that
      would require access to more attributes. One nice thing about it
      is that it gets at the label cost information in the same way
      that the actual shortest-path algorithms (e.g.
      compute_goal_distances_general_cost) do.
    */
    for (const GroupAndTransitions &gat : transition_system) {
        const LabelGroup &label_group = gat.label_group;
        if (label_group.get_cost() != 1 || label_group.get_secondary_cost() != -1)
            return false;
    }
    return true;
}

void Distances::compute_distances(
    bool compute_init_distances,
    bool compute_goal_distances,
    Verbosity verbosity) {
    assert(compute_init_distances || compute_goal_distances);
    /*
      This method does the following:
      - Computes the distances of abstract states from the abstract
        initial state ("abstract g") and to the abstract goal states
        ("abstract h"), depending on the given flags.
    */

    if (are_init_distances_computed()) {
        /*
          The only scenario where distance information is allowed to be
          present when computing distances is when computing goal distances
          for the final transition system in a setting where only init
          distances have been computed during the merge-and-shrink computation.
        */
        assert(!are_goal_distances_computed());
        assert(goal_distances.empty());
        assert(!compute_init_distances);
        assert(compute_goal_distances);
    } else {
        /*
          Otherwise, when computing distances, the previous (invalid)
          distance information must have been cleared before.
        */
        assert(!are_init_distances_computed() && !are_goal_distances_computed());
        assert(init_distances.empty() && goal_distances.empty());
    }

    if (verbosity >= Verbosity::VERBOSE) {
        cout << transition_system.tag();
    }

    int num_states = get_num_states();
    if (num_states == 0) {
        if (verbosity >= Verbosity::VERBOSE) {
            cout << "empty transition system, no distances to compute" << endl;
        }
        init_distances_computed = true;
        return;
    }

    if (compute_init_distances) {
      per_bound_init_distances.resize(num_states, std::vector<std::pair<int,int>>());
    }
    if (compute_goal_distances) {
        per_bound_goal_distances.resize(num_states, std::vector<std::pair<int,int>>());
    }
    if (verbosity >= Verbosity::VERBOSE) {
        cout << "computing ";
        if (compute_init_distances && compute_goal_distances) {
            cout << "init and goal";
        } else if (compute_init_distances) {
            cout << "init";
        } else if (compute_goal_distances) {
            cout << "goal";
        }
        cout << " distances using ";
    }

    if (verbosity >= Verbosity::VERBOSE) {
      cout << "general-cost";
    }
    if (compute_init_distances) {
      compute_init_distances_general_cost();
    }
    if (compute_goal_distances) {
      compute_goal_distances_general_cost();
    }

    if (verbosity >= Verbosity::VERBOSE) {
        cout << " algorithm" << endl;
    }

    if (compute_init_distances) {
        init_distances_computed = true;
    }
    if (compute_goal_distances) {
        goal_distances_computed = true;
    }
}

void Distances::apply_abstraction(
    const StateEquivalenceRelation &state_equivalence_relation,
    bool compute_init_distances,
    bool compute_goal_distances,
    Verbosity verbosity) {
  (void) state_equivalence_relation;
  if (verbosity >= Verbosity::VERBOSE) {
    cout << transition_system.tag()
	 << "simplification was not f-preserving!" << endl;
  }
  clear_distances();
  compute_distances(compute_init_distances, compute_goal_distances, verbosity);
}

void Distances::dump() const {
  /*
    cout << "Bounded cost distances: " << endl;
    for (size_t i = 0; i < init_distances_bounded_cost.size(); ++i) {
        for (size_t j = 0; j < init_distances_bounded_cost[i].size(); ++j) {
            if (init_distances_bounded_cost[i][j] == INF)
                continue;
            cout << i <<  " -> " << j << ": " << init_distances_bounded_cost[i][j] << ", ";
        }
        cout << endl;
    }
    cout << endl;

    cout << "Distances: ";
    for (size_t i = 0; i < goal_distances.size(); ++i) {
        cout << i << ": " << goal_distances[i] << ", ";
    }
    cout << endl;
  */
}

void Distances::statistics() const {
    cout << transition_system.tag();
    if (!are_goal_distances_computed()) {
        cout << "goal distances not computed";
    } else if (transition_system.is_solvable(*this)) {
        cout << "init h=" << get_goal_distance(transition_system.get_init_state());
    } else {
        cout << "transition system is unsolvable";
    }
    cout << endl;
}
}
