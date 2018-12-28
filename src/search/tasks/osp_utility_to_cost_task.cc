#include "osp_utility_to_cost_task.h"

#include "osp_single_end_action_reformulation_task.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"
#include "../tasks/root_task.h"
#include "../utils/system.h"

#include <algorithm>

using namespace std;

namespace extra_tasks {

  OSPUtilityToCostTask::OSPUtilityToCostTask(const shared_ptr<AbstractTask> &parent)
    : DelegatingTask(parent) {
    if (dynamic_cast<OSPSingleEndActionReformulationTask*>(parent.get()) == nullptr) {
      cout << "Parent task is expected to be OSPSingleEndActionReformulationTask.";
      exit(0);
    }

    int sg_index = 0;
    // Operators for each of the variables on which a soft goal is declared.
    for (const auto& var_entry : get_utilities_map()) {
      const int var = var_entry.first;

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

      for (int value = 0; value < parent->get_variable_domain_size(var); ++value) {
	const auto it = var_entry.second.find(value);
	const int var_value_utility = it == var_entry.second.end() ? 0 : it->second;

	sg_operators.emplace_back();

	sg_operators.back().preconditions.push_back(FactPair(var, value));
	sg_operators.back().preconditions.push_back(FactPair(get_sg_variable_index(), sg_index));

	string fact_name = parent->get_fact_name(FactPair(var, value));
	sg_operators.back().name =
	  "account-sg-" + std::to_string(sg_index) + "-" + fact_name;

	sg_operators.back().effects.push_back(FactPair(get_sg_variable_index(), sg_index + 1));

	sg_operators.back().cost = max_utility - var_value_utility;

      }

      sg_index++;
    }
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

  int OSPUtilityToCostTask::get_operator_cost(int index, bool is_axiom) const {
    (void) is_axiom;
    return index < parent->get_num_operators() - 1
      ? 0 // parent->get_operator_cost(index, is_axiom)
      : sg_operators[index - (parent->get_num_operators() - 1)].cost;
  }

  string OSPUtilityToCostTask::get_operator_name(int index, bool is_axiom) const {
    return index < parent->get_num_operators() - 1
      ? parent->get_operator_name(index, is_axiom)
      : sg_operators[index - (parent->get_num_operators() - 1)].name;
  }

  int OSPUtilityToCostTask::get_num_operators() const {
    return (parent->get_num_operators() - 1) + sg_operators.size();
  }

  int OSPUtilityToCostTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    return index < parent->get_num_operators() - 1
      // Extra soft goal precondition.
      ? parent->get_num_operator_preconditions(index, is_axiom) + 1
      : sg_operators[index - (parent->get_num_operators() - 1)].preconditions.size();
  }

  FactPair OSPUtilityToCostTask::get_operator_precondition(
      int op_index, int fact_index, bool is_axiom) const {
    if (op_index < parent->get_num_operators() - 1) {
      return fact_index < parent->get_num_operator_preconditions(op_index, is_axiom)
	? parent->get_operator_precondition(op_index, fact_index, is_axiom)
	: FactPair(get_sg_variable_index(), 0);
    } else {
      return sg_operators[op_index - (parent->get_num_operators() - 1)].preconditions[fact_index];
    }
  }

  int OSPUtilityToCostTask::get_num_operator_effects(int index, bool is_axiom) const {
    return index < parent->get_num_operators() - 1
      ? parent->get_num_operator_effects(index, is_axiom)
      : sg_operators[index - (parent->get_num_operators() - 1)].effects.size();
  }

  int OSPUtilityToCostTask::get_num_operator_effect_conditions(
      int op_index, int eff_index, bool is_axiom) const {
    return op_index < parent->get_num_operators() - 1
      ? parent->get_num_operator_effect_conditions(op_index, eff_index, is_axiom)
      : 0;
  }

  FactPair OSPUtilityToCostTask::get_operator_effect_condition(
      int op_index, int eff_index, int cond_index, bool is_axiom) const {
    return op_index < parent->get_num_operators() - 1
      ? parent->get_operator_effect_condition(op_index, eff_index, cond_index, is_axiom)
      : FactPair::no_fact;
  }

  FactPair OSPUtilityToCostTask::get_operator_effect(
      int op_index, int eff_index, bool is_axiom) const {
    return op_index < parent->get_num_operators() - 1
      ? parent->get_operator_effect(op_index, eff_index, is_axiom)
      : sg_operators[op_index - (parent->get_num_operators() - 1)].effects[eff_index];
  }

  static shared_ptr<AbstractTask> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Utility to cost compilation",
        "Utility to cost compilation.");
    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    } else {
      return make_shared<OSPUtilityToCostTask>(tasks::g_root_task);
    }
}

static Plugin<AbstractTask> _plugin("osp_utility_to_cost", _parse);

}
