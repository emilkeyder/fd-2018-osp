#ifndef TASKS_OSP_DIRECT_UTILITY_TO_COST_TASK_H
#define TASKS_OSP_DIRECT_UTILITY_TO_COST_TASK_H

#include "delegating_task.h"

#include <map>
#include <vector>

namespace extra_tasks {

// HACK: Inheritance here appears to be generic but it is not. We
// assume an OSPSingleEndActionReformulationTask where the END action
// is the last operator.
class OSPDirectUtilityToCostTask : public tasks::DelegatingTask {
protected:
    struct ExplicitOperator {
        std::vector<FactPair> preconditions;
        std::vector<FactPair> effects;
        int cost = 0;
        std::string name;
    };

    const int NOT_AN_AXIOM = -1;

    std::vector<ExplicitOperator> sg_operators;

    int get_sg_variable_index() const {
        // The SG variable is expected to already be present in the parent task.
        return parent->get_num_variables() ;
    }

    int get_sg_variable_domain_size() const {
        return get_utilities_map().size() + 1;
    }

public:
    OSPDirectUtilityToCostTask(const std::shared_ptr<AbstractTask> &parent);
    virtual ~OSPDirectUtilityToCostTask() override = default;


    int get_num_variables() const override;
    std::string get_variable_name(int var) const override;
    int get_variable_domain_size(int var) const override;
    virtual int get_variable_axiom_layer(int var) const override;
    virtual int get_variable_default_axiom_value(int var) const override;
    virtual std::string get_fact_name(const FactPair &fact) const override;
    virtual bool are_facts_mutex(
        const FactPair &fact1, const FactPair &fact2) const override;

    virtual int get_operator_cost(int index, bool is_axiom) const override;
    virtual int get_operator_cost(int index, bool is_axiom, const GlobalState& state) const override;    
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
    virtual int get_num_goals() const override;
    virtual FactPair get_goal_fact(int index) const override;

    virtual int get_bounded_operator_cost(int index, bool is_axiom) const override;
    virtual std::vector<int> get_initial_state_values() const override;

};

}

#endif
