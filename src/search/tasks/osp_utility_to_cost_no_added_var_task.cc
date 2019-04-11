#include "osp_utility_to_cost_no_added_var_task.h"

#include "osp_single_end_action_no_added_var_task.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"
#include "../tasks/root_task.h"
#include "../utils/system.h"

#include <algorithm>

using namespace std;

namespace extra_tasks {

OSPUtilityToCostNoAddedVarTask::OSPUtilityToCostNoAddedVarTask(const shared_ptr<AbstractTask> &parent)
    : DelegatingTask(parent) {
    if (dynamic_cast<OSPSingleEndActionNoAddedVarTask*>(parent.get()) == nullptr) {
        cout << "Parent task is expected to be OSPSingleEndActionNoAddedVarTask.";
        exit(0);
    }

    FactPair previous_var_goal = FactPair::no_fact;

    // Operators for each of the variables on which a soft goal is declared.
    for (const auto& var_entry : get_utilities_map()) {
      const int var = var_entry.first;

      FactPair var_goal(var, parent->get_variable_domain_size(var) - 1);

      /*
      cout << "Considering var " << var << " with possible values: ";
      for (int value = 0; value < parent->get_variable_domain_size(var); ++value) {
        const auto it = var_entry.second.find(value);
       	const int var_value_utility = it == var_entry.second.end() ? 0 : it->second;

      	cout << parent->get_fact_name({var, value}) << " (utility: " << var_value_utility << ") ";
      }
      cout << endl;
      */

      int max_utility = -1;
      for (const auto& util_entry : var_entry.second) {
	if (util_entry.second > max_utility) {
	  max_utility = util_entry.second;
	}
      }
      assert(max_utility > 0);

      for (int value = 0; value < parent->get_variable_domain_size(var) - 1; ++value) {
      	const auto it = var_entry.second.find(value);
	const int var_value_utility = it == var_entry.second.end() ? 0 : it->second;

      	sg_operators.emplace_back();

      	sg_operators.back().preconditions.push_back(FactPair(var, value));
	if (previous_var_goal != FactPair::no_fact) {
	  sg_operators.back().preconditions.push_back(previous_var_goal);
	}

      	sg_operators.back().effects.push_back(var_goal);
      	sg_operators.back().cost = max_utility - var_value_utility;

      	string fact_name = parent->get_fact_name(FactPair(var, value));
      	sg_operators.back().name =
	  "account-sg-" + std::to_string(var) + "-" + fact_name;

	cout << "Fact name: " << fact_name << endl;
	cout << "Operator name: " << sg_operators.back().name << endl;
      }
      previous_var_goal = var_goal;
    }
    cout << "Total extra SG operators: " << sg_operators.size() << endl;
    /*
    for (int i = 0; i < get_num_operators(); ++i) {
      cout << "Operator #" << i << ": " << get_operator_name(i, false) << endl;
      cout << "Preconditions: " << endl;
      for (int j = 0; j < get_num_operator_preconditions(i, false); ++j) {
      	cout << "\t" << get_fact_name(get_operator_precondition(i, j, false)) << endl;
      }
      
      cout << "Effects: " << endl;
      for (int j = 0; j < get_num_operator_effects(i, false); ++j) {
	      cout << "\t" << get_fact_name(get_operator_effect(i, j, false)) << endl;
      }
      cout << endl << endl;
    }
    */
  }

  int OSPUtilityToCostNoAddedVarTask::get_operator_cost(int index, bool is_axiom) const {
    (void) is_axiom;
    return index < parent->get_num_operators() - 1
      ? 0 // parent->get_operator_cost(index, is_axiom)
      : sg_operators[index - (parent->get_num_operators() - 1)].cost;
  }

  string OSPUtilityToCostNoAddedVarTask::get_operator_name(int index, bool is_axiom) const {
    return index < parent->get_num_operators() - 1
      ? parent->get_operator_name(index, is_axiom)
      : sg_operators[index - (parent->get_num_operators() - 1)].name;
  }

  int OSPUtilityToCostNoAddedVarTask::get_num_operators() const {
    return (parent->get_num_operators() - 1) + sg_operators.size();
  }

  int OSPUtilityToCostNoAddedVarTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    return index < parent->get_num_operators() - 1
      // Extra soft goal precondition.
      ? parent->get_num_operator_preconditions(index, is_axiom)
      : sg_operators[index - (parent->get_num_operators() - 1)].preconditions.size();
  }

  FactPair OSPUtilityToCostNoAddedVarTask::get_operator_precondition(
      int op_index, int fact_index, bool is_axiom) const {
    if (op_index < parent->get_num_operators() - 1) {
      return parent->get_operator_precondition(op_index, fact_index, is_axiom);
    } else {
      return sg_operators[op_index - (parent->get_num_operators() - 1)].preconditions[fact_index];
    }
  }

  int OSPUtilityToCostNoAddedVarTask::get_num_operator_effects(int index, bool is_axiom) const {
    return index < parent->get_num_operators() - 1
      ? parent->get_num_operator_effects(index, is_axiom)
      : sg_operators[index - (parent->get_num_operators() - 1)].effects.size();
  }

  int OSPUtilityToCostNoAddedVarTask::get_num_operator_effect_conditions(
      int op_index, int eff_index, bool is_axiom) const {
    return op_index < parent->get_num_operators() - 1
      ? parent->get_num_operator_effect_conditions(op_index, eff_index, is_axiom)
      : 0;
  }

  FactPair OSPUtilityToCostNoAddedVarTask::get_operator_effect_condition(
      int op_index, int eff_index, int cond_index, bool is_axiom) const {
    return op_index < parent->get_num_operators() - 1
      ? parent->get_operator_effect_condition(op_index, eff_index, cond_index, is_axiom)
      : FactPair::no_fact;
  }

  FactPair OSPUtilityToCostNoAddedVarTask::get_operator_effect(
      int op_index, int eff_index, bool is_axiom) const {
    return op_index < parent->get_num_operators() - 1
      ? parent->get_operator_effect(op_index, eff_index, is_axiom)
      : sg_operators[op_index - (parent->get_num_operators() - 1)].effects[eff_index];
  }
  
  static shared_ptr<AbstractTask> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Utility to cost compilation, no added var",
        "Utility to cost compilation, no added var.");
    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    } else {
      return make_shared<OSPUtilityToCostNoAddedVarTask>(tasks::g_root_task);
    }
}

static Plugin<AbstractTask> _plugin("osp_utility_to_cost_no_added_var", _parse);

}
