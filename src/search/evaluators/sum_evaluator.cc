#include "sum_evaluator.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <cassert>
#include <limits>

using namespace std;

namespace sum_evaluator {
SumEvaluator::SumEvaluator(const Options &opts)
    : CombiningEvaluator(opts.get_list<shared_ptr<Evaluator>>("evals")) {
}

SumEvaluator::SumEvaluator(const vector<shared_ptr<Evaluator>> &evals)
    : CombiningEvaluator(evals) {
}

SumEvaluator::~SumEvaluator() {
}

int SumEvaluator::combine_values(const vector<int> &values) {
    int result = 0;
    for (int value : values) {
        assert(value >= 0);
        result += value;
        assert(result >= 0); // Check against overflow.
    }
    return result;
}

static shared_ptr<Evaluator> _parse(OptionParser &parser) {
    parser.document_synopsis("Sum evaluator",
                             "Calculates the sum of the sub-evaluators.");

    parser.add_list_option<Evaluator *>("evals", "at least one evaluator");
    Options opts = parser.parse();

    opts.verify_list_non_empty<Evaluator *>("evals");

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<SumEvaluator>(opts);
}

static PluginShared<Evaluator> _plugin("sum", _parse, "evaluators_basic");
}
