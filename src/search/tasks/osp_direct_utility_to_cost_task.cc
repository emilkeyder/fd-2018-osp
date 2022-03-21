#include "osp_direct_utility_to_cost_task.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"
#include "../tasks/root_task.h"
#include "../utils/system.h"

#include <algorithm>

using namespace std;

namespace extra_tasks {

OSPDirectUtilityToCostTask::OSPDirectUtilityToCostTask(const shared_ptr<AbstractTask> &parent)
    : DelegatingTask(parent) {
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

            if (var_value_utility > 0)
                continue;

            sg_operators.emplace_back();
            sg_operators.back().preconditions.push_back(FactPair(get_sg_variable_index(), sg_index));
            sg_operators.back().name = "account-sg-" + std::to_string(sg_index) + "-no-util";
            sg_operators.back().effects.push_back(FactPair(get_sg_variable_index(), sg_index + 1));
            sg_operators.back().cost = max_utility;
            break;
        }

        for (int value = 0; value < parent->get_variable_domain_size(var); ++value) {
            const auto it = var_entry.second.find(value);
            const int var_value_utility = it == var_entry.second.end() ? 0 : it->second;

            if (var_value_utility == 0)
                continue;

            sg_operators.emplace_back();
            sg_operators.back().preconditions.push_back(FactPair(var, value));
            sg_operators.back().preconditions.push_back(FactPair(get_sg_variable_index(), sg_index));

            const string fact_name = parent->get_fact_name(FactPair(var, value));
            sg_operators.back().name = "account-sg-" + std::to_string(sg_index) + "-" + fact_name;
            sg_operators.back().effects.push_back(FactPair(get_sg_variable_index(), sg_index + 1));
            sg_operators.back().cost = max_utility - var_value_utility;
        }
        cout << "Total extra SG operators: " << sg_operators.size() << endl;
        sg_index++;
    }


    // for (int i = 0; i < get_num_operators(); ++i) {
    //     cout << "Operator #" << i << ": " << get_operator_name(i, false) << endl;
    //     cout << "Preconditions: " << endl;
    //     for (int j = 0; j < get_num_operator_preconditions(i, false); ++j) {
    //         cout << "\t" << get_fact_name(get_operator_precondition(i, j, false)) << endl;
    //     }
      
    //     cout << "Effects: " << endl;
    //     for (int j = 0; j < get_num_operator_effects(i, false); ++j) {
	//         cout << "\t" << get_fact_name(get_operator_effect(i, j, false)) << endl;
    //     }
    //     cout << endl << endl;
    // }
}


int OSPDirectUtilityToCostTask::get_num_variables() const {
    return parent->get_num_variables() + 1;
}

std::string OSPDirectUtilityToCostTask::get_variable_name(int var) const {
    return var < parent->get_num_variables() ? parent->get_variable_name(var) : "sg-index-var";
}

int OSPDirectUtilityToCostTask::get_variable_domain_size(int var) const {
    return var < parent->get_num_variables() ? parent->get_variable_domain_size(var) : get_sg_variable_domain_size();
}

int OSPDirectUtilityToCostTask::get_variable_axiom_layer(int var) const {
    return var < parent->get_num_variables() ? parent->get_variable_axiom_layer(var)
                                             : NOT_AN_AXIOM;
}

int OSPDirectUtilityToCostTask::get_variable_default_axiom_value(int var) const {
    // Return initial state value if sg-index-var.
    return var < parent->get_num_variables() ? parent->get_variable_default_axiom_value(var)
                                             : 0;
}

string OSPDirectUtilityToCostTask::get_fact_name(const FactPair &fact) const {
    if (fact.var < parent->get_num_variables()) {
        return parent->get_fact_name(fact);
    }

    if (fact.var == parent->get_num_variables()) {
        if (fact.value == 0)
            return "sg-init";
        if (fact.value == get_sg_variable_domain_size() - 1)
            return "sg-goal";
        return ("sg-index-" + std::to_string(fact.value));
    }

    return "UNKNOWN FACT: " + std::to_string(fact.var) + ", " + std::to_string(fact.value);
}

bool OSPDirectUtilityToCostTask::are_facts_mutex(
    const FactPair &fact1, const FactPair &fact2) const {
    return fact1.var < parent->get_num_variables() &&
           fact2.var < parent->get_num_variables() &&
            parent->are_facts_mutex(fact1, fact2);
}


int OSPDirectUtilityToCostTask::get_operator_cost(int index, bool is_axiom) const {
    (void)is_axiom;
    return index < parent->get_num_operators()
               ? 0 // parent->get_operator_cost(index, is_axiom)
               : sg_operators[index - parent->get_num_operators()].cost;
}

int OSPDirectUtilityToCostTask::get_operator_cost(int index, bool is_axiom,
                                                           const GlobalState &state) const {
    (void)state;
    return get_operator_cost(index, is_axiom);
}

string OSPDirectUtilityToCostTask::get_operator_name(int index, bool is_axiom) const {
    return index < parent->get_num_operators()
               ? parent->get_operator_name(index, is_axiom)
               : sg_operators[index - parent->get_num_operators()].name;
}

int OSPDirectUtilityToCostTask::get_num_operators() const {
    return parent->get_num_operators() + sg_operators.size();
}

int OSPDirectUtilityToCostTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    return index < parent->get_num_operators()
               // Extra soft goal precondition.
               ? parent->get_num_operator_preconditions(index, is_axiom) + 1
               : sg_operators[index - parent->get_num_operators()].preconditions.size();
}

FactPair OSPDirectUtilityToCostTask::get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const {
    if (op_index < parent->get_num_operators()) {
        return fact_index < parent->get_num_operator_preconditions(op_index, is_axiom)
                   ? parent->get_operator_precondition(op_index, fact_index, is_axiom)
                   : FactPair(get_sg_variable_index(), 0);
    } else {
        return sg_operators[op_index - parent->get_num_operators()].preconditions[fact_index];
    }
}

int OSPDirectUtilityToCostTask::get_num_operator_effects(int index, bool is_axiom) const {
    return index < parent->get_num_operators()
               ? parent->get_num_operator_effects(index, is_axiom)
               : sg_operators[index - parent->get_num_operators()].effects.size();
}

int OSPDirectUtilityToCostTask::get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const {
    return op_index < parent->get_num_operators()
               ? parent->get_num_operator_effect_conditions(op_index, eff_index, is_axiom)
               : 0;
}

FactPair OSPDirectUtilityToCostTask::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    return op_index < parent->get_num_operators()
               ? parent->get_operator_effect_condition(op_index, eff_index, cond_index, is_axiom)
               : FactPair::no_fact;
}

FactPair OSPDirectUtilityToCostTask::get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const {
    return op_index < parent->get_num_operators()
               ? parent->get_operator_effect(op_index, eff_index, is_axiom)
               : sg_operators[op_index - parent->get_num_operators()].effects[eff_index];
}

int OSPDirectUtilityToCostTask::get_bounded_operator_cost(int index, bool is_axiom) const {
    return index < parent->get_num_operators() 
               ? parent->get_operator_cost(index, is_axiom)
               : 0;
}

int OSPDirectUtilityToCostTask::get_num_goals() const {
    return parent->get_num_goals() + 1;
}

FactPair OSPDirectUtilityToCostTask::get_goal_fact(int index) const {
    return index < parent->get_num_goals()
               ? parent->get_goal_fact(index)
               : FactPair(get_sg_variable_index(), get_sg_variable_domain_size() - 1);
}

std::vector<int> OSPDirectUtilityToCostTask::get_initial_state_values() const {
    std::vector<int> initial_values = parent->get_initial_state_values();
    // Initial value of SG variable.
    initial_values.push_back(0);
    return initial_values;
}

static shared_ptr<AbstractTask> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Utility to cost compilation",
        "Utility to cost compilation.");
    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    } else {
        return make_shared<OSPDirectUtilityToCostTask>(tasks::g_root_task);
    }
}

static Plugin<AbstractTask> _plugin("osp_direct_utility_to_cost", _parse);

} // namespace extra_tasks
