#! /usr/bin/env python

"""
Example experiment for the FF planner
(http://fai.cs.uni-saarland.de/hoffmann/ff.html).
"""

import os, sys
import subprocess

from lab.experiment import Experiment
from downward.experiment import FastDownwardExperiment

# In the future, these modules should live in a separate
# "planning" or "solver" package.
from downward import suites
from lab.environments import LocalEnvironment, GridEnvironment
from lab.reports import Attribute, arithmetic_mean, geometric_mean
from downward.reports.absolute import AbsoluteReport
from downward.reports.compare import ComparativeReport
from downward.reports.scatter import ScatterPlotReport
from lab import tools

def get_repo_base():
    """Get base directory of the repository, as an absolute path.

    Search upwards in the directory tree from the main script until a
    directory with a subdirectory named ".hg" is found.

    Abort if the repo base cannot be found."""
    path = os.path.abspath(get_script_dir())
    while os.path.dirname(path) != path:
        if os.path.exists(os.path.join(path, ".hg")):
            return path
        path = os.path.dirname(path)
    sys.exit("repo base could not be found")

class OracleGridEngineEnvironment(GridEnvironment):
    """Abstract base class for grid environments using OGE."""
    # Must be overridden in derived classes.
    DEFAULT_QUEUE = None

    # Can be overridden in derived classes.
    JOB_HEADER_TEMPLATE_FILE = 'oge-job-header'
    RUN_JOB_BODY_TEMPLATE_FILE = 'oge-run-job-body'
    STEP_JOB_BODY_TEMPLATE_FILE = 'oge-step-job-body'
    DEFAULT_PRIORITY = 0
    HOST_RESTRICTIONS = {}
    DEFAULT_HOST_RESTRICTION = ""

    def __init__(self, queue=None, priority=None, host_restriction=None, **kwargs):
        """
        *queue* must be a valid queue name on the grid.

        *priority* must be in the range [-1023, 0] where 0 is the
        highest priority. If you're a superuser the value can be in the
        range [-1023, 1024].

        See :py:class:`~lab.environments.GridEnvironment` for inherited
        parameters.

        """
        GridEnvironment.__init__(self, **kwargs)

        if queue is None:
            queue = self.DEFAULT_QUEUE
        if priority is None:
            priority = self.DEFAULT_PRIORITY
        if host_restriction is None:
            host_restriction = self.DEFAULT_HOST_RESTRICTION

        self.queue = queue
        self.priority = priority
        assert self.priority in xrange(-1023, 1024 + 1)
        self.host_spec = self._get_host_spec(host_restriction)

    # TODO: Don't forget to remove the run-dispatcher file once we get rid
    #       of OracleGridEngineEnvironment.
    def _write_run_dispatcher(self):
        dispatcher_content = tools.fill_template(
            'run-dispatcher.py',
            task_order=self._get_task_order())
        self.exp.add_new_file(
            '', 'run-dispatcher.py', dispatcher_content, permissions=0o755)

    def write_main_script(self):
        # The main script is written by the run_steps() method.
        self._write_run_dispatcher()

    def _get_job_params(self, step, is_last):
        job_params = GridEnvironment._get_job_params(self, step, is_last)
        job_params['priority'] = self.priority
        job_params['queue'] = self.queue
        job_params['host_spec'] = self.host_spec
        job_params['notification'] = '#$ -m n'
        job_params['errfile'] = 'driver.err'

        if is_last and self.email:
            if is_run_step(step):
                logging.warning(
                    "The cluster sends mails per run, not per step."
                    " Since the last of the submitted steps would send"
                    " too many mails, we disable the notification."
                    " We recommend submitting the 'run' step together"
                    " with the 'fetch' step.")
            else:
                job_params['notification'] = '#$ -M %s\n#$ -m e' % self.email

        return job_params

    def _get_host_spec(self, host_restriction):
        if not host_restriction:
            return '## (not used)'
        else:
            hosts = self.HOST_RESTRICTIONS[host_restriction]
            return '#$ -l hostname="%s"' % '|'.join(hosts)

    def _submit_job(self, job_name, job_file, job_dir, dependency=None):
        submit = ['qsub']
        if dependency:
            submit.extend(['-hold_jid', dependency])
        submit.append(job_file)
        tools.run_command(submit, cwd=job_dir)
        return job_name

def get_script():
    """Get file name of main script."""
    return os.path.abspath(sys.argv[0])


def get_script_dir():
    """Get directory of main script.

    Usually a relative directory (depends on how it was called by the user.)"""
    return os.path.dirname(get_script())


def get_base_dir():
    """Assume that this script always lives in the base dir of the infrastructure."""
    return os.path.abspath(get_script_dir())

def get_path_level_up(path):
    return os.path.dirname(path)

def get_planner_dir():
    return get_path_level_up(get_path_level_up(get_base_dir()))


# Change to path to your Fast Downward repository.
REPO = '/storage/US1J6721/experiments/ms-osp'
REPO = get_repo_base()
BENCHMARKS_DIR = '/homes/hny1/US1J6721/software/osp_benchmarks/benchmarks_uniform_utility'
BENCHMARKS_DIR = '/homes/hny1/US1J6721/software/osp_benchmarks/benchmarks_general_utility'

domains = ['airport', 'barman-opt11-strips', 'barman-opt14-strips', 'blocks',
'childsnack-opt14-strips', 'depot', 'driverlog',
'elevators-opt08-strips', 'elevators-opt11-strips',
'floortile-opt11-strips', 'floortile-opt14-strips', 'freecell',
'ged-opt14-strips', 'grid', 'gripper', 'hiking-opt14-strips',
'logistics00', 'logistics98', 'miconic', 'movie', 'mprime', 'mystery',
'nomystery-opt11-strips', 'openstacks-opt08-strips',
'openstacks-opt11-strips', 'openstacks-opt14-strips',
'openstacks-strips', 'parcprinter-08-strips',
'parcprinter-opt11-strips', 'parking-opt11-strips',
'parking-opt14-strips', 'pathways-noneg', 'pegsol-08-strips',
'pegsol-opt11-strips', 'pipesworld-notankage', 'pipesworld-tankage',
'psr-small', 'rovers', 'satellite', 'scanalyzer-08-strips',
'scanalyzer-opt11-strips', 'sokoban-opt08-strips',
'sokoban-opt11-strips', 'storage', 'tetris-opt14-strips',
'tidybot-opt11-strips', 'tidybot-opt14-strips', 'tpp',
'transport-opt08-strips', 'transport-opt11-strips',
'transport-opt14-strips', 'trucks-strips', 'visitall-opt11-strips',
'visitall-opt14-strips', 'woodworking-opt08-strips',
'woodworking-opt11-strips', 'zenotravel']


bounds = ['25', '50', '75', '100']


#domains = ['blocks', 'driverlog', 'tpp', 'trucks']
#domains = ['zenotravel']
#bounds = ['25']
#bounds = ['75', '100']

SUITE = []
for x in domains:
    for y in bounds:
        SUITE.append('%s-%s' % (x,y))

   
ENV = OracleGridEngineEnvironment(queue='all.q')

# Create a new experiment.
#exp = Experiment(environment=ENV)
exp = FastDownwardExperiment(environment=ENV)

# Add built-in parsers.
#exp.add_parser(exp.LAB_STATIC_PROPERTIES_PARSER)
#exp.add_parser(exp.LAB_DRIVER_PARSER)
#exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
#exp.add_parser(exp.PLANNER_PARSER)

# Add custom parser.
exp.add_parser('osp_parser.py')



config_name = 'adapted-search-all-translator-general-util'
config_date = '19-10-10'
report_name = '%s-%s' % (config_name,config_date)
exppath = os.path.join('data', report_name)


exp.add_suite(BENCHMARKS_DIR, SUITE)

REV='a02669afc6ba'


# Add step that writes experiment files to disk.
#exp.add_step('build', exp.build)

# Add step that executes all runs.
#exp.add_step('start', exp.start_runs)

# Add step that collects properties from run directories and
# writes them to *-eval/properties.
#exp.add_fetcher(name='fetch')

exp.add_fetcher('data/2019-02-16-tr-adapted-search-blind-eval')
exp.add_fetcher('data/2019-02-16-tr-adapted-search-hmax-eval')
exp.add_fetcher('data/2019-10-08-tr-adapted-search-ms-eval')
exp.add_fetcher('/storage/US1J6721/lab/examples/osp/data/2019-02-17-blind-bnb-translator-osp-eval')

ATTRIBUTES = ['coverage', 'expansions', 'evaluations', 'expansions_until_last_jump', 'reopened', 'search_time', 'total_time', 'util_initial_state', 'util_upper_bound', 'plan_util', 'initial_h_value']


def rename_algorithms(run):
    name = run['algorithm']
    paper_names = {
        'bnb-blind' : 'BnB blind',
        'ref-adapted-search-tr-blind' : 'blind',
        'ref-adapted-search-hmax-cb' : 'hmax-cb',
        'ref-adapted-search-hmax-nocb' : 'hmax',
        'ref-adapted-search-ms-lr-cb-600' : 'MS-cb',
        'ref-adapted-search-ms-lr-nocb-600' : 'MS'
}
    run['algorithm'] = paper_names[name]
    return run

algorithms=['BnB blind', 'blind', 'hmax-cb', 'hmax', 'MS-cb', 'MS']
# Make a report.
exp.add_report(
    AbsoluteReport(attributes=ATTRIBUTES, filter=rename_algorithms, filter_algorithm=algorithms),
    outfile='%s.html' % report_name)

def filter_domains(run, collection):
    dom = run['domain']
    if dom.endswith(collection):
        return True
    return False

def filter_domains_100(run):
    return filter_domains(run, '100')

def filter_domains_75(run):
    return filter_domains(run, '75')

def filter_domains_50(run):
    return filter_domains(run, '50')

def filter_domains_25(run):
    return filter_domains(run, '25')

def get_func_name(category):
    if category is '100':
        return filter_domains_100
    if category is '75':
        return filter_domains_75
    if category is '50':
        return filter_domains_50
    if category is '25':
        return filter_domains_25

def bound_as_category(run1, run2):
    dom = run1['domain']
    if dom.endswith('100'):
        return '100'
    if dom.endswith('75'):
        return '75'
    if dom.endswith('50'):
        return '50'
    if dom.endswith('25'):
        return '25'
    return 'None'

def add_scatterplots(attr, alg1, alg2, category=None):
    if category:
        exp.add_report(ScatterPlotReport(attributes=[attr],
             filter=[rename_algorithms,get_func_name(category)],
             filter_algorithm=[alg1, alg2],
             get_category=bound_as_category,
             #format="png",  # Use "tex" for pgfplots output.
             format="tex",  # Use "tex" for pgfplots output.
             ),
             name="scatterplot-%s-%s-%s-%s" % (category, attr, alg1, alg2))
    else:
        exp.add_report(ScatterPlotReport(attributes=[attr],
             filter=rename_algorithms,
             filter_algorithm=[alg1, alg2],
             get_category=bound_as_category,
             #format="png",  # Use "tex" for pgfplots output.
             format="tex",  # Use "tex" for pgfplots output.
             ),
             name="scatterplot-%s-%s-%s" % (attr, alg1, alg2))

#for attr in ["expansions_until_last_jump", "expansions"]:
for attr in ["expansions_until_last_jump"]:
    for category in ["100", "75", "50", "25", None]:
        add_scatterplots(attr, "blind", "hmax-cb", category)
        add_scatterplots(attr, "blind", 'MS-cb', category)
        add_scatterplots(attr, 'MS', 'MS-cb', category)
        add_scatterplots(attr, 'hmax', 'hmax-cb', category)


for category in ["100", "75", "50", "25"]:
    exp.add_report(
    AbsoluteReport(attributes=["coverage"], 
             filter=[rename_algorithms,get_func_name(category)],
             filter_algorithm=algorithms, format="tex"),
    outfile='coverage-%s-%s.tex' % (report_name, category))


#for attr in ["expansions_until_last_jump", "expansions"]:
for attr in ["expansions_until_last_jump"]:
    for category in ["100", "75", "50", "25", None]:
        add_scatterplots(attr, "BnB blind", "blind", category)

##Parse the commandline and run the specified steps.
exp.run_steps()
