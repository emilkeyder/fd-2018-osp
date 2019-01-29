#include "max_heuristic.h"

#include "../global_state.h"
#include "../option_parser.h"
#include "../plugin.h"

#include <cassert>
#include <limits>
#include <vector>
using namespace std;

namespace max_heuristic {
/*
  TODO: At the time of this writing, this shares huge amounts of code
        with h^add, and the two should be refactored so that the
        common code is only included once, in so far as this is
        possible without sacrificing run-time. We may want to avoid
        virtual calls in the inner-most loops; maybe a templated
        strategy pattern is an option. Right now, the only differences
        to the h^add code are the use of max() instead of add() and
        the lack of preferred operator support (but we might actually
        reintroduce that if it doesn't hurt performance too much).
 */

// construction and destruction
HSPMaxHeuristic::HSPMaxHeuristic(const Options &opts)
  : RelaxationHeuristic(opts), use_cost_bound(opts.get<bool>("use_cost_bound")) {
    cout << "Initializing HSP max heuristic..." << endl;
    cout << "use_cost_bound = " << (use_cost_bound ? "true" : "false") << endl;
}

HSPMaxHeuristic::~HSPMaxHeuristic() {
}

// heuristic computation
void HSPMaxHeuristic::setup_exploration_queue(int bound) {
    queue.clear();

    for (vector<Proposition> &props_of_var : propositions) {
        for (Proposition &prop : props_of_var) {
            prop.cost = -1;
	    prop.bounded_cost = -1;
        }
    }

    // Deal with operators and axioms without preconditions.
    for (UnaryOperator &op : unary_operators) {
        op.unsatisfied_preconditions = op.precondition.size();
        op.cost = op.base_cost; // will be increased by precondition costs
	op.bounded_cost = op.base_bounded_cost;

        if (op.unsatisfied_preconditions == 0 &&
	    (!use_cost_bound || op.bounded_cost <= bound)) {
            enqueue_if_necessary(op.effect, op.cost, op.bounded_cost);
	}
    }
}

void HSPMaxHeuristic::setup_exploration_queue_state(const State &state) {
    for (FactProxy fact : state) {
        Proposition *init_prop = get_proposition(fact);
        enqueue_if_necessary(init_prop, 0, 0);
    }
}

void HSPMaxHeuristic::relaxed_exploration(int cost_bound) {
    int unsolved_goals = goal_propositions.size();
    while (!queue.empty()) {
        pair<int, Proposition *> top_pair = queue.pop();
        int distance = top_pair.first;
        Proposition *prop = top_pair.second;

        int prop_cost = prop->cost;
	int prop_bounded_cost = prop->bounded_cost;

	// May have decreased since we enqueued it.
        assert(prop_cost <= distance);

// 	cout << prop_names_dict[prop->id]
// 	     << " with cost " << prop_cost << ", bounded cost " << prop_bounded_cost << endl;

	if (use_cost_bound && prop_bounded_cost > cost_bound) {
// 	  cout << "Skipping proposition " << prop_names_dict[prop->id]
// 	       << " with bounded cost " << prop_bounded_cost << endl;
	  continue;
	}
        if (prop_cost < distance) {
            continue;
	}
        if (prop->is_goal && --unsolved_goals == 0) {
            return;
	}
        const vector<UnaryOperator *> &triggered_operators =
            prop->precondition_of;
        for (UnaryOperator *unary_op : triggered_operators) {
            --unary_op->unsatisfied_preconditions;
            unary_op->cost = max(unary_op->cost,
                                 unary_op->base_cost + prop_cost);
	    unary_op->bounded_cost = max(unary_op->bounded_cost, 
					 unary_op->base_bounded_cost + prop_bounded_cost);

// 	    cout << "Unary op " << task_proxy.get_operators()[unary_op->operator_no].get_name()
// 		 << " with cost " << unary_op->cost << " and bounded cost "
// 		 << unary_op->bounded_cost << endl;

            assert(unary_op->unsatisfied_preconditions >= 0);
            if (unary_op->unsatisfied_preconditions == 0) {
	      enqueue_if_necessary(unary_op->effect, unary_op->cost, unary_op->bounded_cost);
	    }
        }
    }
}

int HSPMaxHeuristic::compute_heuristic_w_bound(
    const GlobalState &global_state, int cost_bound) {
  return compute_heuristic(global_state, cost_bound);
}

int HSPMaxHeuristic::compute_heuristic(const GlobalState &global_state) {
  return compute_heuristic(global_state, std::numeric_limits<int>::max());
}

int HSPMaxHeuristic::compute_heuristic(const GlobalState &global_state, int cost_bound) {
//    cout << "Evaluating state with cost bound " << cost_bound << endl;
//    global_state.dump_pddl();

    const State state = convert_global_state(global_state);

    int total_cost = 0;

    setup_exploration_queue(cost_bound);
    setup_exploration_queue_state(state);
    relaxed_exploration(cost_bound);
    
    total_cost = 0;
    for (Proposition *prop : goal_propositions) {
      int prop_cost = prop->cost;
      if (prop_cost == -1) {
	return DEAD_END;
      }
      total_cost = max(total_cost, prop_cost);
    }

    return total_cost;
}

  void HSPMaxHeuristic::notify_state_transition(
    const GlobalState &parent_state, OperatorID op_id,
    const GlobalState &state) {
    (void) parent_state;
    (void) op_id;
    if (cache_evaluator_values && use_cost_bound) {
        /* TODO:  It may be more efficient to check that the reached landmark
           set has actually changed and only then mark the h value as dirty. */
        heuristic_cache[state].dirty = true;
    }
  }

static shared_ptr<Heuristic> _parse(OptionParser &parser) {
    parser.document_synopsis("Max heuristic", "");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "supported");
    parser.document_language_support(
        "axioms",
        "supported (in the sense that the planner won't complain -- "
        "handling of axioms might be very stupid "
        "and even render the heuristic unsafe)");
    parser.document_property("admissible", "yes for tasks without axioms");
    parser.document_property("consistent", "yes for tasks without axioms");
    parser.document_property("safe", "yes for tasks without axioms");
    parser.document_property("preferred operators", "no");

    parser.add_option<bool>("use_cost_bound",
			    "Use or ignore passed in secondary cost bound", "true");

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<HSPMaxHeuristic>(opts);
}


static Plugin<Evaluator> _plugin("hmax", _parse);
}
