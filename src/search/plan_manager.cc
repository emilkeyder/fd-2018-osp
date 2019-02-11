#include "plan_manager.h"

#include "task_proxy.h"

#include "state_registry.h"
#include "task_utils/task_properties.h"

#include <fstream>
#include <iostream>
#include <sstream>


using namespace std;

int calculate_plan_cost(const Plan &plan, const TaskProxy &task_proxy) {
    OperatorsProxy operators = task_proxy.get_operators();
    int plan_cost = 0;
    for (OperatorID op_id : plan) {
        plan_cost += operators[op_id].get_cost();
    }
    return plan_cost;
}

int calculate_bounded_plan_cost(const Plan &plan, const TaskProxy &task_proxy) {
    OperatorsProxy operators = task_proxy.get_operators();
    int plan_cost = 0;
    for (OperatorID op_id : plan) {
        plan_cost += operators[op_id].get_bounded_cost();
    }
    return plan_cost;
}

PlanManager::PlanManager()
    : plan_filename("sas_plan"),
      num_previously_generated_plans(0),
      is_part_of_anytime_portfolio(false) {
}

void PlanManager::set_plan_filename(const string &plan_filename_) {
    plan_filename = plan_filename_;
}

void PlanManager::set_num_previously_generated_plans(int num_previously_generated_plans_) {
    num_previously_generated_plans = num_previously_generated_plans_;
}

void PlanManager::set_is_part_of_anytime_portfolio(bool is_part_of_anytime_portfolio_) {
    is_part_of_anytime_portfolio = is_part_of_anytime_portfolio_;
}

bool is_soft_goal_action(const OperatorProxy& op) {
  std::vector<string> soft_goal_ops = { "SG_END_ACTION" };
  for (const string& soft_goal_op : soft_goal_ops) {
    if (op.get_name() == soft_goal_op) return true;
  }
  return false;
}

void PlanManager::save_plan(
    const Plan &plan, const TaskProxy &task_proxy,
    bool generates_multiple_plan_files) {
    ostringstream filename;
    filename << plan_filename;
    int plan_number = num_previously_generated_plans + 1;
    if (generates_multiple_plan_files || is_part_of_anytime_portfolio) {
        filename << "." << plan_number;
    } else {
        assert(plan_number == 1);
    }
    ofstream outfile(filename.str());
    if (outfile.rdstate() & ofstream::failbit) {
        cerr << "Failed to open plan file: " << filename.str() << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }

    StateRegistry plan_states_registry(task_proxy);
    GlobalState gs = plan_states_registry.get_initial_state();

    OperatorsProxy operators = task_proxy.get_operators();
    for (OperatorID op_id : plan) {
      if (!is_soft_goal_action(operators[op_id])) {
        cout << operators[op_id].get_name() << " (" << operators[op_id].get_bounded_cost() << ")" << endl;
        outfile << "(" << operators[op_id].get_name() << ")" << endl;
	gs = plan_states_registry.get_successor_state(gs, operators[op_id]);
      }
    }

    int plan_cost = calculate_bounded_plan_cost(plan, task_proxy);
    bool is_unit_cost = task_properties::is_unit_cost(task_proxy);

    outfile << "; cost = " << plan_cost << " ("
            << (is_unit_cost ? "unit cost" : "general cost") << ")" << endl;
    outfile.close();

    int plan_size = 0;
    for (const OperatorID op_id : plan) {
      if (!is_soft_goal_action(operators[op_id])) ++plan_size;
    }

    cout << "Plan length: " << plan_size << " step(s)." << endl;
    cout << "Plan cost: " << plan_cost << endl;

    int plan_utility = task_proxy.get_state_utility(gs);
    cout << "Solution found with utility value: " << plan_utility << endl;

    ++num_previously_generated_plans;
}
