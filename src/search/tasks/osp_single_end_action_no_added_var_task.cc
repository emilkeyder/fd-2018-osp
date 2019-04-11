#include "osp_single_end_action_no_added_var_task.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"
#include "../tasks/root_task.h"
#include "../utils/system.h"

#include <algorithm>

using namespace std;

namespace extra_tasks {

  OSPSingleEndActionNoAddedVarTask::OSPSingleEndActionNoAddedVarTask(const shared_ptr<AbstractTask> &parent)
    : DelegatingTask(parent) {

    end_operator.cost = 0;
    for (const auto& var_entry : get_utilities_map()) {
      int max_utility = -1;
      const int var = var_entry.first;
      sg_vars_vec.push_back(var);
      end_operator.effects.push_back(FactPair(var, parent->get_variable_domain_size(var)));

      for (const auto& util_entry : var_entry.second) {
	if (util_entry.second > max_utility) {
	  max_utility = util_entry.second;
	}
      }
      assert(max_utility > 0);

      end_operator.cost += max_utility;
    }

    for (int i = 0; i < parent->get_num_goals(); ++i) {
      end_operator.preconditions.push_back(parent->get_goal_fact(i));
    }

    // Cost is actually variable, but will be updated within
    // search. Set it to the worst caseOB to ensure that heuristic
    // computation never prefers it to incremental collect/forgo
    // actions.

    for (const FactPairUtility& fpu : get_fact_pair_utilities()) {
      end_operator.cost += fpu.utility;
    }
    end_operator.name = "SG_END_ACTION";
  }

  int OSPSingleEndActionNoAddedVarTask::get_num_variables() const {
    return parent->get_num_variables();
  }

  std::string OSPSingleEndActionNoAddedVarTask::get_variable_name(int var) const {
    return var < parent->get_num_variables() ? parent->get_variable_name(var) : "sg-index-var";
  }

  int OSPSingleEndActionNoAddedVarTask::get_variable_domain_size(int var) const {
    return get_utilities_map().find(var) == get_utilities_map().end() ?
      parent->get_variable_domain_size(var) : parent->get_variable_domain_size(var) + 1;
  }

  int OSPSingleEndActionNoAddedVarTask::get_variable_axiom_layer(int var) const {
    return var < parent->get_num_variables() ? parent->get_variable_axiom_layer(var)
      : NOT_AN_AXIOM;
  }

  int OSPSingleEndActionNoAddedVarTask::get_variable_default_axiom_value(int var) const {
    // Return initial state value if sg-index-var.
    return var < parent->get_num_variables() ? parent->get_variable_default_axiom_value(var)
      : 0;
  }

  string OSPSingleEndActionNoAddedVarTask::get_fact_name(const FactPair& fp) const {
    if (fp.value == parent->get_variable_domain_size(fp.var)) {
      return "var-" + std::to_string(fp.var) + "-sg";
    }
    return parent->get_fact_name(fp);
  }

  bool OSPSingleEndActionNoAddedVarTask::are_facts_mutex(
      const FactPair &fact1, const FactPair &fact2) const {
    return fact1.var < parent->get_num_variables() &&
      fact2.var < parent->get_num_variables() &&
      parent->are_facts_mutex(fact1, fact2);
  }

  int OSPSingleEndActionNoAddedVarTask::get_operator_cost(int index, bool is_axiom) const {
    (void) is_axiom;
    return index < parent->get_num_operators()
      ? 0 // parent->get_operator_cost(index, is_axiom)
      : end_operator.cost;
  }

  int OSPSingleEndActionNoAddedVarTask::get_operator_cost(int index, bool is_axiom,
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

  string OSPSingleEndActionNoAddedVarTask::get_operator_name(int index, bool is_axiom) const {
    if (index < parent->get_num_operators()) {
      return parent->get_operator_name(index, is_axiom);
    } else if (index == parent->get_num_operators()) {
      return end_operator.name;
    }

    cerr << "No operator with index " << index << " in get_operator_name(), this is a bug." << endl;
    return "";
  }

  int OSPSingleEndActionNoAddedVarTask::get_num_operators() const {
    return parent->get_num_operators() + 1;
  }

  int OSPSingleEndActionNoAddedVarTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    if (index < parent->get_num_operators()) {
      return parent->get_num_operator_preconditions(index, is_axiom);
    } else if (index == parent->get_num_operators()) {
      return end_operator.preconditions.size();
    }

    cerr << "No operator with index " << index
	 << " in get_num_operator_preconditions, this is a bug." << endl;
    return 0;
  }

  FactPair OSPSingleEndActionNoAddedVarTask::get_operator_precondition(
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

  int OSPSingleEndActionNoAddedVarTask::get_num_operator_effects(int index, bool is_axiom) const {
    if (index < parent->get_num_operators()) {
      return parent->get_num_operator_effects(index, is_axiom);
    } else if (index == parent->get_num_operators()) {
      return end_operator.effects.size();
    }

    cerr << "No operator with index " << index
	 << " in get_num_operator_effects, this is a bug." << endl;
    return 0;
  }

  int OSPSingleEndActionNoAddedVarTask::get_num_operator_effect_conditions(
      int op_index, int eff_index, bool is_axiom) const {
    return op_index < parent->get_num_operators()
      ? parent->get_num_operator_effect_conditions(op_index, eff_index, is_axiom)
      : 0;
  }

  FactPair OSPSingleEndActionNoAddedVarTask::get_operator_effect_condition(
      int op_index, int eff_index, int cond_index, bool is_axiom) const {
    return op_index < parent->get_num_operators()
      ? parent->get_operator_effect_condition(op_index, eff_index, cond_index, is_axiom)
      : FactPair::no_fact;
  }

  FactPair OSPSingleEndActionNoAddedVarTask::get_operator_effect(
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

  int OSPSingleEndActionNoAddedVarTask::get_num_goals() const {
    return sg_vars_vec.size();
  }

  FactPair OSPSingleEndActionNoAddedVarTask::get_goal_fact(int index) const {
    const int var = sg_vars_vec[index];
    return FactPair(var, parent->get_variable_domain_size(var));
  }

  int OSPSingleEndActionNoAddedVarTask::get_bounded_operator_cost(int index, bool is_axiom) const {
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
      return make_shared<OSPSingleEndActionNoAddedVarTask>(tasks::g_root_task);
    }
}

static Plugin<AbstractTask> _plugin("osp_single_end_action_no_added_var", _parse);

}
