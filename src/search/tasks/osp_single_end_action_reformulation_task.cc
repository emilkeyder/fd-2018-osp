#include "osp_single_end_action_reformulation_task.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"
#include "../tasks/root_task.h"
#include "../utils/system.h"

#include <algorithm>

using namespace std;

namespace extra_tasks {

  OSPSingleEndActionReformulationTask::OSPSingleEndActionReformulationTask(const shared_ptr<AbstractTask> &parent)
    : DelegatingTask(parent) {
    for (int i = 0; i < parent->get_num_goals(); ++i) {
      end_operator.preconditions.push_back(parent->get_goal_fact(i));
    }

    for (int var = 0; var < parent->get_num_variables(); ++var) {
      end_operator.effects.push_back(FactPair(var, parent->get_initial_state_values()[var]));
      // end_operator.effects.push_back(FactPair(var, 0));
    }

    end_operator.effects.push_back(FactPair(get_sg_variable_index(),
					    get_sg_variable_domain_size() - 1));
    // Cost is actually variable, but will be updated within
    // search. Set it to the sum of all utilities to ensure that
    // heuristic computation never prefers it to incremental
    // collect/forgo actions.
    end_operator.cost = 0;
    for (const FactPairUtility& fpu : get_fact_pair_utilities()) {
      end_operator.cost += fpu.utility;
    }
    end_operator.name = "SG_END_ACTION";
  }

  int OSPSingleEndActionReformulationTask::get_num_variables() const {
    return parent->get_num_variables() + 1;
  }

  std::string OSPSingleEndActionReformulationTask::get_variable_name(int var) const {
    return var < parent->get_num_variables() ? parent->get_variable_name(var) : "sg-index-var";
  }

  int OSPSingleEndActionReformulationTask::get_variable_domain_size(int var) const {
    return var < parent->get_num_variables() ? parent->get_variable_domain_size(var) :
      get_sg_variable_domain_size();
  }

  int OSPSingleEndActionReformulationTask::get_variable_axiom_layer(int var) const {
    return var < parent->get_num_variables() ? parent->get_variable_axiom_layer(var)
      : NOT_AN_AXIOM;
  }

  int OSPSingleEndActionReformulationTask::get_variable_default_axiom_value(int var) const {
    // Return initial state value if sg-index-var.
    return var < parent->get_num_variables() ? parent->get_variable_default_axiom_value(var)
      : 0;
  }

  string OSPSingleEndActionReformulationTask::get_fact_name(const FactPair& fact) const {
    if (fact.var < parent->get_num_variables()) {
      return parent->get_fact_name(fact);
    }

    if (fact.var == parent->get_num_variables()) {
      if (fact.value == 0) return "sg-init";
      if (fact.value == get_sg_variable_domain_size() - 1) return "sg-goal";
      return ("sg-index-" + std::to_string(fact.value));
    }

    return "UNKNOWN FACT: " + std::to_string(fact.var) + ", " + std::to_string(fact.value);
  }

  bool OSPSingleEndActionReformulationTask::are_facts_mutex(
      const FactPair &fact1, const FactPair &fact2) const {
    return fact1.var < parent->get_num_variables() &&
      fact2.var < parent->get_num_variables() &&
      parent->are_facts_mutex(fact1, fact2);
  }

  int OSPSingleEndActionReformulationTask::get_operator_cost(int index, bool is_axiom) const {
    (void) is_axiom;
    return index < parent->get_num_operators()
      ? 0 // parent->get_operator_cost(index, is_axiom)
      : end_operator.cost;
  }

  int OSPSingleEndActionReformulationTask::get_operator_cost(int index, bool is_axiom,
							     const GlobalState& state) const {
    (void) is_axiom;
    if (index < parent->get_num_operators()) {
      return 0;
    }

    if (index == parent->get_num_operators()) {
      return get_max_possible_utility() - get_state_utility(state);
    }

    cerr << "No operator with index " << index << " in get_operator_cost(), this is a bug." << endl;
    return 0;
  }

  string OSPSingleEndActionReformulationTask::get_operator_name(int index, bool is_axiom) const {
    if (index < parent->get_num_operators()) {
      return parent->get_operator_name(index, is_axiom);
    } else if (index == parent->get_num_operators()) {
      return end_operator.name;
    }

    cerr << "No operator with index " << index << " in get_operator_name(), this is a bug." << endl;
    return "";
  }

  int OSPSingleEndActionReformulationTask::get_num_operators() const {
    return parent->get_num_operators() + 1;
  }

  int OSPSingleEndActionReformulationTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    if (index < parent->get_num_operators()) {
      return parent->get_num_operator_preconditions(index, is_axiom);
    } else if (index == parent->get_num_operators()) {
      return end_operator.preconditions.size();
    }

    cerr << "No operator with index " << index
	 << " in get_num_operator_preconditions, this is a bug." << endl;
    return 0;
  }

  FactPair OSPSingleEndActionReformulationTask::get_operator_precondition(
      int op_index, int fact_index, bool is_axiom) const {
    if (op_index < parent->get_num_operators()) {
      return parent->get_operator_precondition(op_index, fact_index, is_axiom);
    } else if (op_index == parent->get_num_operators()) {
      return end_operator.preconditions[fact_index];
    }

    cerr << "No operator with index " << op_index
	 << " in get_operator_precondition, this is a bug." << endl;
    return FactPair::no_fact;
  }

  int OSPSingleEndActionReformulationTask::get_num_operator_effects(int index, bool is_axiom) const {
    if (index < parent->get_num_operators()) {
      return parent->get_num_operator_effects(index, is_axiom);
    } else if (index == parent->get_num_operators()) {
      return end_operator.effects.size();
    }

    cerr << "No operator with index " << index
	 << " in get_num_operator_effects, this is a bug." << endl;
    return 0;
  }

  int OSPSingleEndActionReformulationTask::get_num_operator_effect_conditions(
      int op_index, int eff_index, bool is_axiom) const {
    return op_index < parent->get_num_operators()
      ? parent->get_num_operator_effect_conditions(op_index, eff_index, is_axiom)
      : 0;
  }

  FactPair OSPSingleEndActionReformulationTask::get_operator_effect_condition(
      int op_index, int eff_index, int cond_index, bool is_axiom) const {
    return op_index < parent->get_num_operators()
      ? parent->get_operator_effect_condition(op_index, eff_index, cond_index, is_axiom)
      : FactPair::no_fact;
  }

  FactPair OSPSingleEndActionReformulationTask::get_operator_effect(
      int op_index, int eff_index, bool is_axiom) const {
    if (op_index < parent->get_num_operators()) {
      return parent->get_operator_effect(op_index, eff_index, is_axiom);
    } else if (op_index == parent->get_num_operators()) {
      return end_operator.effects[eff_index];
    }

    cerr << "No operator with index " << op_index
	 << " in get_operator_effect(), this is a bug." << endl;
    return FactPair::no_fact;
  }

  int OSPSingleEndActionReformulationTask::get_num_goals() const {
    return 1;
  }

  FactPair OSPSingleEndActionReformulationTask::get_goal_fact(int index) const {
    return index < parent->get_num_goals()
      ? parent->get_goal_fact(index)
      : FactPair(get_sg_variable_index(), get_sg_variable_domain_size() - 1);
  }

  std::vector<int> OSPSingleEndActionReformulationTask::get_initial_state_values() const {
    std::vector<int> initial_values = parent->get_initial_state_values();
    // Initial value of SG variable.
    initial_values.push_back(0);
    return initial_values;
  }

  int OSPSingleEndActionReformulationTask::get_bounded_operator_cost(int index, bool is_axiom) const {
    return index < parent->get_num_operators()
      ? parent->get_operator_cost(index, is_axiom) : 0;
  }

  static shared_ptr<AbstractTask> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Utility to cost compilation, single end action with state-dependent cost.",
        "Utility to cost compilation, single end action with state-dependent cost.");
    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    } else {
      return make_shared<OSPSingleEndActionReformulationTask>(tasks::g_root_task);
    }
}

static Plugin<AbstractTask> _plugin("osp_single_end_action", _parse);

}
