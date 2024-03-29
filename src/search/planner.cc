#include "option_parser.h"
#include "search_engine.h"

#include "options/registries.h"
#include "tasks/root_task.h"
#include "task_utils/task_properties.h"
#include "utils/system.h"
#include "utils/timer.h"

#include "tasks/osp_single_end_action_reformulation_task.h"
#include "tasks/osp_direct_utility_to_cost_task.h"

#include <iostream>

using namespace std;
using utils::ExitCode;

int main(int argc, const char **argv) {
    utils::register_event_handlers();

    if (argc < 2) {
        cout << options::usage(argv[0]) << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }

    bool unit_cost = false;
    if (static_cast<string>(argv[1]) != "--help") {
        cout << "reading input... [t=" << utils::g_timer << "]" << endl;
        tasks::read_root_task(cin);
        cout << "done reading input! [t=" << utils::g_timer << "]" << endl;
	
	// cout << "Doing OSPSingleEndActionReformulationTask conversion... [t=" << utils::g_timer << "]" << endl;
	// tasks::g_root_task = std::make_shared<extra_tasks::OSPSingleEndActionReformulationTask>(
	//     tasks::g_root_task);
	// cout << "Done with OSPSingleEndActionReformulationTask conversion [t=" << utils::g_timer << "]" << endl;


	cout << "Doing OSPDirectUtilityToCostTask conversion... [t=" << utils::g_timer << "]" << endl;
	tasks::g_root_task = std::make_shared<extra_tasks::OSPDirectUtilityToCostTask>(
	    tasks::g_root_task);
	cout << "Done with OSPDirectUtilityToCostTask conversion [t=" << utils::g_timer << "]" << endl;

	std::vector<FactPairUtility> utilities = tasks::g_root_task->get_fact_pair_utilities();
	std::vector<int> initial_state_values = tasks::g_root_task->get_initial_state_values();

	int initial_state_util = 0;
	for (const FactPairUtility& fpu : utilities) {
	  if (initial_state_values[fpu.fact_pair.var] == fpu.fact_pair.value) {
	    initial_state_util += fpu.utility;
	  }
	}
	
	cout << "Initial State has utility: " << initial_state_util << endl;
	cout << "Utility upper-bound: " << tasks::g_root_task->get_max_possible_utility() << endl;
	
        TaskProxy task_proxy(*tasks::g_root_task);
        unit_cost = task_properties::is_unit_cost(task_proxy);
    }
    
    shared_ptr<SearchEngine> engine;

    // The command line is parsed twice: once in dry-run mode, to
    // check for simple input errors, and then in normal mode.
    try {
        options::Registry &registry = *options::Registry::instance();
        options::parse_cmd_line(argc, argv, registry, true, unit_cost);
        engine = options::parse_cmd_line(argc, argv, registry, false, unit_cost);
    } catch (ArgError &error) {
        cerr << error << endl;
        options::usage(argv[0]);
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    } catch (ParseError &error) {
        cerr << error << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }

    for (int i = 0; i < tasks::g_root_task->get_num_goals(); ++i) {
      cout << "Goal #" << i << ": "
	   << tasks::g_root_task->get_fact_name(tasks::g_root_task->get_goal_fact(i)) << endl;
    }

    utils::Timer search_timer;
    engine->search();
    search_timer.stop();
    utils::g_timer.stop();

    engine->save_plan_if_necessary();
    engine->print_statistics();
    cout << "Search time: " << search_timer << endl;
    cout << "Total time: " << utils::g_timer << endl;

    if (engine->found_solution()) {
        utils::exit_with(ExitCode::SUCCESS);
    } else {
        utils::exit_with(ExitCode::SEARCH_UNSOLVED_INCOMPLETE);
    }
}
