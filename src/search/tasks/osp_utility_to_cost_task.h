#ifndef TASKS_OSP_UTILITY_TO_COST_TASK_H
#define TASKS_OSP_UTILITY_TO_COST_TASK_H

#include "delegating_task.h"

#include <map>
#include <vector>

namespace extra_tasks {

// HACK: Inheritance here appears to be generic but it is not. We
// assume an OSPSingleEndActionReformulationTask where the END action
// is the last operator.
class OSPUtilityToCostTask : public tasks::DelegatingTask {
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
        return parent->get_num_variables() - 1;
    }

    int get_sg_variable_domain_size() const {
        return get_utilities_map().size() + 1;
    }

public:
    OSPUtilityToCostTask(const std::shared_ptr<AbstractTask> &parent);
    virtual ~OSPUtilityToCostTask() override = default;

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
};

}

#endif
