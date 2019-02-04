#include "bounded_g_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace bounded_g_evaluator {
EvaluationResult BoundedGEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    result.set_evaluator_value(eval_context.get_bounded_g_value());
    return result;
}

static shared_ptr<Evaluator> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "bounded g-value evaluator",
        "Returns the bounded g-value (path cost) of the search node.");
    parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<BoundedGEvaluator>();
}

static Plugin<Evaluator> _plugin("bounded_g", _parse, "evaluators_basic");
}
