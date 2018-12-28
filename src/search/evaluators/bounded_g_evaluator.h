#ifndef EVALUATORS_BOUNDED_G_EVALUATOR_H
#define EVALUATORS_BOUNDED_G_EVALUATOR_H

#include "../evaluator.h"

namespace bounded_g_evaluator {
class BoundedGEvaluator : public Evaluator {
public:
    BoundedGEvaluator() = default;
    virtual ~BoundedGEvaluator() override = default;

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &) override {}
};
}

#endif
