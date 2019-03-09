#include "shrink_by_h.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "transition_system.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/collections.h"
#include "../utils/hash.h"
#include "../utils/markup.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

using namespace std;

namespace merge_and_shrink {
ShrinkByH::ShrinkByH(const Options &) {
}

StateEquivalenceRelation ShrinkByH::compute_equivalence_relation(
        const TransitionSystem &ts,
        const Distances &distances,
        int ) const {
    assert(distances.are_goal_distances_computed());
    int max_h = 0;
//    cout << "Dumping all distances" << endl;
    for (int state = 0; state < ts.get_size(); ++state) {
        int h = distances.get_goal_distance(state);
        if (h != INF)
            max_h = max(max_h, h);
//        cout << "Distance: " << h << ", current maximum: " << max_h << endl;
    }
    int num_states = ts.get_size();
    // If max_h == 0, then we don't shrink
    bool has_goal_variables = (max_h > 0);
    if (has_goal_variables) {
        typedef utils::HashMap<std::vector<std::pair<int,int> >, std::vector<int> > GroupMap;
        GroupMap buckets;

        int bucket_count = 0;
        for (int state = 0; state < num_states; ++state) {
	        const std::vector<std::pair<int,int>>& per_bound_dists = distances.get_per_bound_distances(state);
	        vector<int> &bucket = buckets[per_bound_dists];

	        if (!bucket.empty()) ++bucket_count;
	  
	        bucket.push_back(state);
        }
        StateEquivalenceRelation equiv_relation;
        equiv_relation.reserve(bucket_count);

    	for (auto it = buckets.begin(); it != buckets.end(); ++it) {
	        const vector<int>& bucket = it->second;
	        equiv_relation.push_back(StateEquivalenceClass());
	        StateEquivalenceClass &group = equiv_relation.back();
	        group.insert_after(group.before_begin(), bucket.begin(), bucket.end());
        }
        assert(static_cast<int>(equiv_relation.size()) == bucket_count);
        return equiv_relation;
    }
    // No goal variables: no shrinking, each state is a separate equivalence class
    StateEquivalenceRelation equiv_relation;
    equiv_relation.resize(num_states);
    for (int state = 0; state < num_states; ++state) {
        StateEquivalenceClass &group = equiv_relation[state];
        group.push_front(state);
    }
    return equiv_relation;

}

string ShrinkByH::name() const {
    return "MS-lite style";
}

void ShrinkByH::dump_strategy_specific_options() const {

}

static shared_ptr<ShrinkStrategy>_parse(OptionParser &parser) {
    parser.document_synopsis(
        "MS-lite style shrink strategy",
        "This shrink strategy implements the algorithm described in"
        " the MS-lite paper, with one exception: there is no shrinking of "
        "components without goal variables");
    parser.document_note(
        "shrink_by_h()",
        "Shrinking all abstract state with the same h value.");
    ShrinkBucketBased::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.help_mode())
        return nullptr;

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<ShrinkByH>(opts);
}

static Plugin<ShrinkStrategy> _plugin("shrink_by_h", _parse);
}
