#ifndef TASKS_OSP_UTILITY_TO_COST_TASK_H
#define TASKS_OSP_UTILITY_TO_COST_TASK_H

#include "delegating_task.h"

#include <vector>

namespace extra_tasks {

class OSPUtilityToCostTask : public tasks::DelegatingTask {

  public:
    OSPUtilityToCostTask(const std::shared_ptr<AbstractTask> &parent);
    virtual ~OSPUtilityToCostTask() override = default;

    int get_bounded_operator_cost(int index, bool is_axiom) const override;
};

}

#endif
