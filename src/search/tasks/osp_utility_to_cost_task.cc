#include "osp_utility_to_cost_task.h"

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
    for (const auto util : parent->get_fact_pair_utilities()) {
      utilities_map[util.fact_pair.var][util.fact_pair.value] = util.utility;
      cout << "Fact " << parent->get_fact_name({util.fact_pair.var, util.fact_pair.value}) << " with utility " << util.utility << endl;
    }

    int sg_index = 0;
    // Operators for each of the variables on which a soft goal is declared.
    for (const auto& var_entry : utilities_map) {
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

	/*
	cout << "Added operator " << sg_operators.back().name
	     << " (" << sg_operators.back().cost << ")"
	     << endl << "\twith preconditions: ";
	for (size_t i = 0; i < sg_operators.back().preconditions.size(); ++i) {
	  cout << get_fact_name(sg_operators.back().preconditions[i]) << ", ";
	}
	cout << endl << "\teffects: ";
	for (size_t i = 0; i < sg_operators.back().effects.size(); ++i) {
	  cout << get_fact_name(sg_operators.back().effects[i]) << ", ";
	}
	cout << endl << endl;
	*/
      }

      sg_index++;
    }
    
  }

  int OSPUtilityToCostTask::get_num_variables() const {
    return parent->get_num_variables() + 1;
  }

  std::string OSPUtilityToCostTask::get_variable_name(int var) const {
    return var < parent->get_num_variables() ? parent->get_variable_name(var) : "sg-index-var";
  }

  int OSPUtilityToCostTask::get_variable_domain_size(int var) const {
    return var < parent->get_num_variables() ? parent->get_variable_domain_size(var) : 
      get_sg_variable_domain_size();
  }

  int OSPUtilityToCostTask::get_variable_axiom_layer(int var) const {
    return var < parent->get_num_variables() ? parent->get_variable_axiom_layer(var)
      : NOT_AN_AXIOM;
  }

  int OSPUtilityToCostTask::get_variable_default_axiom_value(int var) const {
    // Return initial state value if sg-index-var.
    return var < parent->get_num_variables() ? parent->get_variable_default_axiom_value(var)
      : 0;
  }

  string OSPUtilityToCostTask::get_fact_name(const FactPair& fact) const {
    if (fact.var < parent->get_num_variables()) {
      return parent->get_fact_name(fact);
    }

    const string value_string = fact.value == 0 ? "ongoing-plan-execution"
      : fact.value == get_variable_domain_size(fact.var) - 1 ? "goal"
      : "index-" + std::to_string(fact.value);
    return "sg-" + value_string;
  }

  bool OSPUtilityToCostTask::are_facts_mutex(
      const FactPair &fact1, const FactPair &fact2) const {
    return fact1.var < parent->get_num_variables() &&
      fact2.var < parent->get_num_variables() &&
      parent->are_facts_mutex(fact1, fact2);
  }

  int OSPUtilityToCostTask::get_operator_cost(int index, bool is_axiom) const {
    return index < parent->get_num_operators() ? parent->get_operator_cost(index, is_axiom)
      : sg_operators[index - parent->get_num_operators()].cost;
  }

  string OSPUtilityToCostTask::get_operator_name(int index, bool is_axiom) const {
    return index < parent->get_num_operators() ? parent->get_operator_name(index, is_axiom)
      : sg_operators[index - parent->get_num_operators()].name;
  }

  int OSPUtilityToCostTask::get_num_operators() const {
    return parent->get_num_operators() + sg_operators.size();
  }

  int OSPUtilityToCostTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    return index < parent->get_num_operators()
      // Extra soft goal precondition.
      ? parent->get_num_operator_preconditions(index, is_axiom) + 1
      : sg_operators[index - parent->get_num_operators()].preconditions.size();
  }

  FactPair OSPUtilityToCostTask::get_operator_precondition(
      int op_index, int fact_index, bool is_axiom) const {
    if (op_index < parent->get_num_operators()) {
      return fact_index < parent->get_num_operator_preconditions(op_index, is_axiom)
	? parent->get_operator_precondition(op_index, fact_index, is_axiom)
	: FactPair(get_sg_variable_index(), 0);
    } else {
      return sg_operators[op_index - parent->get_num_operators()].preconditions[fact_index];
    }
  }

  int OSPUtilityToCostTask::get_num_operator_effects(int index, bool is_axiom) const {
    return index < parent->get_num_operators()
      ? parent->get_num_operator_effects(index, is_axiom)
      : sg_operators[index - parent->get_num_operators()].effects.size();
  }

  int OSPUtilityToCostTask::get_num_operator_effect_conditions(
      int op_index, int eff_index, bool is_axiom) const {
    return op_index < parent->get_num_operators()
      ? parent->get_num_operator_effect_conditions(op_index, eff_index, is_axiom)
      : 0;
  }

  FactPair OSPUtilityToCostTask::get_operator_effect_condition(
      int op_index, int eff_index, int cond_index, bool is_axiom) const {
    return op_index < parent->get_num_operators()
      ? parent->get_operator_effect_condition(op_index, eff_index, cond_index, is_axiom)
      : FactPair::no_fact;
  }

  FactPair OSPUtilityToCostTask::get_operator_effect(
      int op_index, int eff_index, bool is_axiom) const {
    return op_index < parent->get_num_operators()
      ? parent->get_operator_effect(op_index, eff_index, is_axiom)
      : sg_operators[op_index - parent->get_num_operators()].effects[eff_index];
  }

  int OSPUtilityToCostTask::get_num_goals() const { 
    return parent->get_num_goals() + 1;
  }

  FactPair OSPUtilityToCostTask::get_goal_fact(int index) const {
    return index < parent->get_num_goals() 
      ? parent->get_goal_fact(index)
      : FactPair(get_sg_variable_index(), get_sg_variable_domain_size() - 1);
  }

  std::vector<int> OSPUtilityToCostTask::get_initial_state_values() const {
    std::vector<int> initial_values = parent->get_initial_state_values();
    // Initial value of SG variable.
    initial_values.push_back(0);
    return initial_values;
  }

  int OSPUtilityToCostTask::get_bounded_operator_cost(int index, bool is_axiom) const {
    return index < parent->get_num_operators()
      ? parent->get_operator_cost(index, is_axiom) : 0;
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
