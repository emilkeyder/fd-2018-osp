#include "osp_utility_to_cost_task.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"
#include "../tasks/root_task.h"
#include "../utils/system.h"

using namespace std;

namespace extra_tasks {

  OSPUtilityToCostTask::OSPUtilityToCostTask(const shared_ptr<AbstractTask> &parent)
    : DelegatingTask(parent) {
    cout << "Instantiating OSPUtilityToCostTask" << endl;
  }

  int OSPUtilityToCostTask::get_bounded_operator_cost(int index, bool is_axiom) const {
    return parent->get_operator_cost(index, is_axiom);
  }

  static shared_ptr<AbstractTask> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Utility to cost compilation",
        "Utility to cost compilation.");
    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    } else {
      return make_shared<OSPUtilityToCostTask>(tasks::g_root_task);
    }
}

static Plugin<AbstractTask> _plugin("osp_utility_to_cost", _parse);

}
