#ifndef TASKS_OSP_UTILITY_TO_COST_TASK_H
#define TASKS_OSP_UTILITY_TO_COST_TASK_H

#include "delegating_task.h"

#include <map>
#include <vector>

namespace extra_tasks {

class OSPUtilityToCostTask : public tasks::DelegatingTask {

    struct ExplicitOperator {
      std::vector<FactPair> preconditions;
      std::vector<FactPair> effects;
      int cost = 0;
      std::string name;
    };


    // variable index --> var value --> utility
    std::map<int, std::map<int, int>> utilities_map;
    std::vector<ExplicitOperator> sg_operators;

    const int NOT_AN_AXIOM = -1;

    int get_sg_variable_index() const {
      return parent->get_num_variables();
    }

    int get_sg_variable_domain_size() const {
      return utilities_map.size() + 1;
    }

  public:
    OSPUtilityToCostTask(const std::shared_ptr<AbstractTask> &parent);
    virtual ~OSPUtilityToCostTask() override = default;

    int get_num_variables() const override;
    std::string get_variable_name(int var) const override;
    int get_variable_domain_size(int var) const override; 
    virtual int get_variable_axiom_layer(int var) const override;
    virtual int get_variable_default_axiom_value(int var) const override;
    virtual std::string get_fact_name(const FactPair &fact) const override;
    virtual bool are_facts_mutex(
        const FactPair &fact1, const FactPair &fact2) const override;

    virtual int get_operator_cost(int index, bool is_axiom) const override;
    virtual std::string get_operator_name(int index, bool is_axiom) const override;
    virtual int get_num_operators() const override;
    virtual int get_num_operator_preconditions(int index, bool is_axiom) const override;
    virtual FactPair get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    virtual int get_num_operator_effects(int op_index, bool is_axiom) const override;
    virtual int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;
    // virtual int convert_operator_index(
    //     int index, const AbstractTask *ancestor_task) const final override;
    // virtual int convert_operator_index_to_parent(int index) const {
    //    return index;
    // }

    virtual int get_num_goals() const override;
    virtual FactPair get_goal_fact(int index) const override;

    virtual std::vector<int> get_initial_state_values() const override;

/*     virtual int get_cost_bound() const override; */
/*     virtual std::vector<FactPairUtility> get_fact_pair_utilities() const override; */

    virtual int get_bounded_operator_cost(int index, bool is_axiom) const override;   
};

}

#endif
