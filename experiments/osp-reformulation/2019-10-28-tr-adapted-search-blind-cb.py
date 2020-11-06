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



tmp_dict = {u'ref-adapted-search-ms-lr-cb-600': [u'airport-25:p50-airport5MUC-p15.pddl', u'airport-50:p50-airport5MUC-p15.pddl', u'airport-75:p50-airport5MUC-p15.pddl', u'airport-100:p50-airport5MUC-p15.pddl', u'barman-opt11-strips-25:pfile05-018.pddl', u'barman-opt11-strips-50:pfile05-018.pddl', u'barman-opt11-strips-75:pfile05-018.pddl', u'barman-opt11-strips-100:pfile05-018.pddl', u'elevators-opt08-strips-25:p29.pddl', u'floortile-opt11-strips-25:opt-p07-013.pddl', u'floortile-opt11-strips-25:opt-p07-014.pddl', u'floortile-opt11-strips-25:opt-p10-019.pddl', u'floortile-opt11-strips-50:opt-p06-011.pddl', u'floortile-opt11-strips-50:opt-p07-013.pddl', u'floortile-opt11-strips-50:opt-p07-014.pddl', u'floortile-opt11-strips-50:opt-p10-019.pddl', u'floortile-opt11-strips-75:opt-p06-011.pddl', u'floortile-opt11-strips-75:opt-p07-013.pddl', u'floortile-opt11-strips-75:opt-p07-014.pddl', u'floortile-opt11-strips-75:opt-p10-019.pddl', u'floortile-opt11-strips-100:opt-p06-011.pddl', u'floortile-opt11-strips-100:opt-p07-013.pddl', u'floortile-opt11-strips-100:opt-p07-014.pddl', u'floortile-opt11-strips-100:opt-p10-019.pddl', u'floortile-opt14-strips-25:p01-4-4-2.pddl', u'floortile-opt14-strips-25:p01-5-5-2.pddl', u'floortile-opt14-strips-25:p01-6-4-2.pddl', u'floortile-opt14-strips-25:p03-5-3-2.pddl', u'floortile-opt14-strips-25:p03-6-4-2.pddl', u'floortile-opt14-strips-50:p01-6-4-2.pddl', u'floortile-opt14-strips-50:p03-6-4-2.pddl', u'floortile-opt14-strips-75:p01-6-4-2.pddl', u'floortile-opt14-strips-75:p03-6-4-2.pddl', u'floortile-opt14-strips-100:p01-6-4-2.pddl', u'floortile-opt14-strips-100:p03-6-4-2.pddl', u'freecell-25:probfreecell-5-3.pddl', u'freecell-50:probfreecell-4-1.pddl', u'ged-opt14-strips-75:d-8-9.pddl', u'grid-50:prob02.pddl', u'grid-75:prob02.pddl', u'logistics98-25:prob22.pddl', u'logistics98-25:prob27.pddl', u'logistics98-25:prob28.pddl', u'logistics98-25:prob29.pddl', u'logistics98-50:prob22.pddl', u'logistics98-50:prob27.pddl', u'logistics98-50:prob28.pddl', u'logistics98-50:prob29.pddl', u'logistics98-75:prob22.pddl', u'logistics98-75:prob27.pddl', u'logistics98-75:prob28.pddl', u'logistics98-75:prob29.pddl', u'logistics98-100:prob22.pddl', u'logistics98-100:prob27.pddl', u'logistics98-100:prob28.pddl', u'logistics98-100:prob29.pddl', u'mprime-25:prob18.pddl', u'mprime-50:prob10.pddl', u'mprime-50:prob22.pddl', u'mprime-50:prob30.pddl', u'mprime-75:prob15.pddl', u'mprime-75:prob20.pddl', u'mprime-100:prob08.pddl', u'mprime-100:prob16.pddl', u'parcprinter-08-strips-50:p19.pddl', u'parcprinter-08-strips-75:p19.pddl', u'parcprinter-08-strips-100:p19.pddl', u'parcprinter-opt11-strips-50:p17.pddl', u'parcprinter-opt11-strips-75:p19.pddl', u'parcprinter-opt11-strips-100:p19.pddl', u'satellite-25:p29-HC-pfile9.pddl', u'satellite-25:p30-HC-pfile10.pddl', u'satellite-25:p31-HC-pfile11.pddl', u'satellite-25:p32-HC-pfile12.pddl', u'satellite-25:p33-HC-pfile13.pddl', u'satellite-25:p34-HC-pfile14.pddl', u'satellite-25:p35-HC-pfile15.pddl', u'satellite-25:p36-HC-pfile16.pddl', u'satellite-50:p29-HC-pfile9.pddl', u'satellite-50:p30-HC-pfile10.pddl', u'satellite-50:p31-HC-pfile11.pddl', u'satellite-50:p32-HC-pfile12.pddl', u'satellite-50:p33-HC-pfile13.pddl', u'satellite-50:p34-HC-pfile14.pddl', u'satellite-50:p35-HC-pfile15.pddl', u'satellite-50:p36-HC-pfile16.pddl', u'satellite-75:p29-HC-pfile9.pddl', u'satellite-75:p30-HC-pfile10.pddl', u'satellite-75:p31-HC-pfile11.pddl', u'satellite-75:p32-HC-pfile12.pddl', u'satellite-75:p33-HC-pfile13.pddl', u'satellite-75:p34-HC-pfile14.pddl', u'satellite-75:p35-HC-pfile15.pddl', u'satellite-75:p36-HC-pfile16.pddl', u'satellite-100:p29-HC-pfile9.pddl', u'satellite-100:p30-HC-pfile10.pddl', u'satellite-100:p31-HC-pfile11.pddl', u'satellite-100:p32-HC-pfile12.pddl', u'satellite-100:p33-HC-pfile13.pddl', u'satellite-100:p34-HC-pfile14.pddl', u'satellite-100:p35-HC-pfile15.pddl', u'satellite-100:p36-HC-pfile16.pddl', u'scanalyzer-08-strips-25:p07.pddl', u'scanalyzer-08-strips-25:p10.pddl', u'scanalyzer-08-strips-25:p11.pddl', u'scanalyzer-08-strips-25:p13.pddl', u'scanalyzer-08-strips-25:p14.pddl', u'scanalyzer-08-strips-25:p16.pddl', u'scanalyzer-08-strips-25:p18.pddl', u'scanalyzer-08-strips-25:p19.pddl', u'scanalyzer-08-strips-25:p20.pddl', u'scanalyzer-08-strips-25:p21.pddl', u'scanalyzer-08-strips-25:p29.pddl', u'scanalyzer-08-strips-25:p30.pddl', u'scanalyzer-08-strips-50:p07.pddl', u'scanalyzer-08-strips-50:p10.pddl', u'scanalyzer-08-strips-50:p11.pddl', u'scanalyzer-08-strips-50:p13.pddl', u'scanalyzer-08-strips-50:p14.pddl', u'scanalyzer-08-strips-50:p16.pddl', u'scanalyzer-08-strips-50:p18.pddl', u'scanalyzer-08-strips-50:p19.pddl', u'scanalyzer-08-strips-50:p20.pddl', u'scanalyzer-08-strips-50:p21.pddl', u'scanalyzer-08-strips-50:p28.pddl', u'scanalyzer-08-strips-50:p29.pddl', u'scanalyzer-08-strips-50:p30.pddl', u'scanalyzer-08-strips-75:p07.pddl', u'scanalyzer-08-strips-75:p10.pddl', u'scanalyzer-08-strips-75:p11.pddl', u'scanalyzer-08-strips-75:p13.pddl', u'scanalyzer-08-strips-75:p14.pddl', u'scanalyzer-08-strips-75:p16.pddl', u'scanalyzer-08-strips-75:p18.pddl', u'scanalyzer-08-strips-75:p19.pddl', u'scanalyzer-08-strips-75:p20.pddl', u'scanalyzer-08-strips-75:p21.pddl', u'scanalyzer-08-strips-75:p29.pddl', u'scanalyzer-08-strips-75:p30.pddl', u'scanalyzer-08-strips-100:p07.pddl', u'scanalyzer-08-strips-100:p10.pddl', u'scanalyzer-08-strips-100:p11.pddl', u'scanalyzer-08-strips-100:p13.pddl', u'scanalyzer-08-strips-100:p14.pddl', u'scanalyzer-08-strips-100:p16.pddl', u'scanalyzer-08-strips-100:p18.pddl', u'scanalyzer-08-strips-100:p19.pddl', u'scanalyzer-08-strips-100:p20.pddl', u'scanalyzer-08-strips-100:p21.pddl', u'scanalyzer-08-strips-100:p29.pddl', u'scanalyzer-08-strips-100:p30.pddl', u'scanalyzer-opt11-strips-25:p05.pddl', u'scanalyzer-opt11-strips-25:p07.pddl', u'scanalyzer-opt11-strips-25:p08.pddl', u'scanalyzer-opt11-strips-25:p09.pddl', u'scanalyzer-opt11-strips-25:p10.pddl', u'scanalyzer-opt11-strips-25:p15.pddl', u'scanalyzer-opt11-strips-25:p16.pddl', u'scanalyzer-opt11-strips-25:p17.pddl', u'scanalyzer-opt11-strips-25:p19.pddl', u'scanalyzer-opt11-strips-50:p05.pddl', u'scanalyzer-opt11-strips-50:p07.pddl', u'scanalyzer-opt11-strips-50:p08.pddl', u'scanalyzer-opt11-strips-50:p09.pddl', u'scanalyzer-opt11-strips-50:p10.pddl', u'scanalyzer-opt11-strips-50:p15.pddl', u'scanalyzer-opt11-strips-50:p16.pddl', u'scanalyzer-opt11-strips-50:p17.pddl', u'scanalyzer-opt11-strips-50:p19.pddl', u'scanalyzer-opt11-strips-75:p05.pddl', u'scanalyzer-opt11-strips-75:p07.pddl', u'scanalyzer-opt11-strips-75:p08.pddl', u'scanalyzer-opt11-strips-75:p09.pddl', u'scanalyzer-opt11-strips-75:p10.pddl', u'scanalyzer-opt11-strips-75:p15.pddl', u'scanalyzer-opt11-strips-75:p16.pddl', u'scanalyzer-opt11-strips-75:p17.pddl', u'scanalyzer-opt11-strips-75:p19.pddl', u'scanalyzer-opt11-strips-100:p05.pddl', u'scanalyzer-opt11-strips-100:p07.pddl', u'scanalyzer-opt11-strips-100:p08.pddl', u'scanalyzer-opt11-strips-100:p09.pddl', u'scanalyzer-opt11-strips-100:p10.pddl', u'scanalyzer-opt11-strips-100:p15.pddl', u'scanalyzer-opt11-strips-100:p16.pddl', u'scanalyzer-opt11-strips-100:p17.pddl', u'scanalyzer-opt11-strips-100:p19.pddl', u'storage-25:p21.pddl', u'tetris-opt14-strips-25:p03-8.pddl', u'tetris-opt14-strips-25:p03-10.pddl', u'tetris-opt14-strips-25:p04-10.pddl', u'tetris-opt14-strips-25:p05-8.pddl', u'tetris-opt14-strips-25:p05-10.pddl', u'tetris-opt14-strips-50:p03-8.pddl', u'tetris-opt14-strips-50:p03-10.pddl', u'tetris-opt14-strips-50:p04-10.pddl', u'tetris-opt14-strips-50:p05-8.pddl', u'tetris-opt14-strips-50:p05-10.pddl', u'tetris-opt14-strips-75:p03-8.pddl', u'tetris-opt14-strips-75:p03-10.pddl', u'tetris-opt14-strips-75:p04-10.pddl', u'tetris-opt14-strips-75:p05-8.pddl', u'tetris-opt14-strips-75:p05-10.pddl', u'tetris-opt14-strips-100:p03-8.pddl', u'tetris-opt14-strips-100:p03-10.pddl', u'tetris-opt14-strips-100:p04-10.pddl', u'tetris-opt14-strips-100:p05-8.pddl', u'tetris-opt14-strips-100:p05-10.pddl', u'transport-opt08-strips-25:p15.pddl', u'transport-opt08-strips-25:p28.pddl', u'transport-opt08-strips-25:p29.pddl', u'transport-opt08-strips-50:p05.pddl', u'transport-opt11-strips-25:p10.pddl', u'transport-opt11-strips-25:p12.pddl', u'transport-opt11-strips-25:p20.pddl', u'transport-opt11-strips-50:p17.pddl', u'transport-opt11-strips-75:p17.pddl', u'transport-opt11-strips-100:p17.pddl', u'transport-opt14-strips-50:p04.pddl', u'transport-opt14-strips-50:p08.pddl', u'transport-opt14-strips-50:p15.pddl', u'transport-opt14-strips-75:p15.pddl', u'transport-opt14-strips-100:p14.pddl', u'transport-opt14-strips-100:p15.pddl', u'woodworking-opt08-strips-50:p19.pddl'], u'ref-adapted-search-ms-lr-nocb-600': [u'airport-25:p50-airport5MUC-p15.pddl', u'airport-50:p50-airport5MUC-p15.pddl', u'airport-75:p50-airport5MUC-p15.pddl', u'airport-100:p50-airport5MUC-p15.pddl', u'barman-opt11-strips-25:pfile05-018.pddl', u'barman-opt11-strips-50:pfile05-018.pddl', u'barman-opt11-strips-75:pfile05-018.pddl', u'barman-opt11-strips-100:pfile05-018.pddl', u'floortile-opt11-strips-25:opt-p06-011.pddl', u'floortile-opt11-strips-25:opt-p07-013.pddl', u'floortile-opt11-strips-25:opt-p07-014.pddl', u'floortile-opt11-strips-25:opt-p10-019.pddl', u'floortile-opt11-strips-50:opt-p06-011.pddl', u'floortile-opt11-strips-50:opt-p07-013.pddl', u'floortile-opt11-strips-50:opt-p07-014.pddl', u'floortile-opt11-strips-50:opt-p10-019.pddl', u'floortile-opt11-strips-75:opt-p06-011.pddl', u'floortile-opt11-strips-75:opt-p07-013.pddl', u'floortile-opt11-strips-75:opt-p07-014.pddl', u'floortile-opt11-strips-75:opt-p10-019.pddl', u'floortile-opt11-strips-100:opt-p06-011.pddl', u'floortile-opt11-strips-100:opt-p07-013.pddl', u'floortile-opt11-strips-100:opt-p07-014.pddl', u'floortile-opt11-strips-100:opt-p10-019.pddl', u'floortile-opt14-strips-25:p01-6-4-2.pddl', u'floortile-opt14-strips-25:p03-6-4-2.pddl', u'floortile-opt14-strips-50:p01-6-4-2.pddl', u'floortile-opt14-strips-50:p03-6-4-2.pddl', u'floortile-opt14-strips-75:p01-6-4-2.pddl', u'floortile-opt14-strips-75:p03-6-4-2.pddl', u'floortile-opt14-strips-100:p01-6-4-2.pddl', u'floortile-opt14-strips-100:p03-6-4-2.pddl', u'logistics98-25:prob22.pddl', u'logistics98-25:prob27.pddl', u'logistics98-25:prob28.pddl', u'logistics98-25:prob29.pddl', u'logistics98-50:prob22.pddl', u'logistics98-50:prob27.pddl', u'logistics98-50:prob28.pddl', u'logistics98-50:prob29.pddl', u'logistics98-75:prob22.pddl', u'logistics98-75:prob27.pddl', u'logistics98-75:prob28.pddl', u'logistics98-75:prob29.pddl', u'logistics98-100:prob22.pddl', u'logistics98-100:prob27.pddl', u'logistics98-100:prob28.pddl', u'logistics98-100:prob29.pddl', u'parcprinter-08-strips-25:p19.pddl', u'parcprinter-08-strips-50:p19.pddl', u'parcprinter-08-strips-75:p19.pddl', u'parcprinter-08-strips-100:p19.pddl', u'parcprinter-opt11-strips-25:p19.pddl', u'parcprinter-opt11-strips-50:p19.pddl', u'parcprinter-opt11-strips-75:p19.pddl', u'parcprinter-opt11-strips-100:p19.pddl', u'satellite-25:p29-HC-pfile9.pddl', u'satellite-25:p30-HC-pfile10.pddl', u'satellite-25:p31-HC-pfile11.pddl', u'satellite-25:p32-HC-pfile12.pddl', u'satellite-25:p33-HC-pfile13.pddl', u'satellite-25:p34-HC-pfile14.pddl', u'satellite-25:p35-HC-pfile15.pddl', u'satellite-25:p36-HC-pfile16.pddl', u'satellite-50:p29-HC-pfile9.pddl', u'satellite-50:p30-HC-pfile10.pddl', u'satellite-50:p31-HC-pfile11.pddl', u'satellite-50:p32-HC-pfile12.pddl', u'satellite-50:p33-HC-pfile13.pddl', u'satellite-50:p34-HC-pfile14.pddl', u'satellite-50:p35-HC-pfile15.pddl', u'satellite-50:p36-HC-pfile16.pddl', u'satellite-75:p29-HC-pfile9.pddl', u'satellite-75:p30-HC-pfile10.pddl', u'satellite-75:p31-HC-pfile11.pddl', u'satellite-75:p32-HC-pfile12.pddl', u'satellite-75:p33-HC-pfile13.pddl', u'satellite-75:p34-HC-pfile14.pddl', u'satellite-75:p35-HC-pfile15.pddl', u'satellite-75:p36-HC-pfile16.pddl', u'satellite-100:p29-HC-pfile9.pddl', u'satellite-100:p30-HC-pfile10.pddl', u'satellite-100:p31-HC-pfile11.pddl', u'satellite-100:p32-HC-pfile12.pddl', u'satellite-100:p33-HC-pfile13.pddl', u'satellite-100:p34-HC-pfile14.pddl', u'satellite-100:p35-HC-pfile15.pddl', u'satellite-100:p36-HC-pfile16.pddl', u'scanalyzer-08-strips-25:p07.pddl', u'scanalyzer-08-strips-25:p10.pddl', u'scanalyzer-08-strips-25:p11.pddl', u'scanalyzer-08-strips-25:p13.pddl', u'scanalyzer-08-strips-25:p14.pddl', u'scanalyzer-08-strips-25:p16.pddl', u'scanalyzer-08-strips-25:p18.pddl', u'scanalyzer-08-strips-25:p19.pddl', u'scanalyzer-08-strips-25:p20.pddl', u'scanalyzer-08-strips-25:p21.pddl', u'scanalyzer-08-strips-25:p29.pddl', u'scanalyzer-08-strips-25:p30.pddl', u'scanalyzer-08-strips-50:p07.pddl', u'scanalyzer-08-strips-50:p10.pddl', u'scanalyzer-08-strips-50:p11.pddl', u'scanalyzer-08-strips-50:p13.pddl', u'scanalyzer-08-strips-50:p14.pddl', u'scanalyzer-08-strips-50:p16.pddl', u'scanalyzer-08-strips-50:p18.pddl', u'scanalyzer-08-strips-50:p19.pddl', u'scanalyzer-08-strips-50:p20.pddl', u'scanalyzer-08-strips-50:p21.pddl', u'scanalyzer-08-strips-50:p29.pddl', u'scanalyzer-08-strips-50:p30.pddl', u'scanalyzer-08-strips-75:p07.pddl', u'scanalyzer-08-strips-75:p10.pddl', u'scanalyzer-08-strips-75:p11.pddl', u'scanalyzer-08-strips-75:p13.pddl', u'scanalyzer-08-strips-75:p14.pddl', u'scanalyzer-08-strips-75:p16.pddl', u'scanalyzer-08-strips-75:p18.pddl', u'scanalyzer-08-strips-75:p19.pddl', u'scanalyzer-08-strips-75:p20.pddl', u'scanalyzer-08-strips-75:p21.pddl', u'scanalyzer-08-strips-75:p29.pddl', u'scanalyzer-08-strips-75:p30.pddl', u'scanalyzer-08-strips-100:p07.pddl', u'scanalyzer-08-strips-100:p10.pddl', u'scanalyzer-08-strips-100:p11.pddl', u'scanalyzer-08-strips-100:p13.pddl', u'scanalyzer-08-strips-100:p14.pddl', u'scanalyzer-08-strips-100:p16.pddl', u'scanalyzer-08-strips-100:p18.pddl', u'scanalyzer-08-strips-100:p19.pddl', u'scanalyzer-08-strips-100:p20.pddl', u'scanalyzer-08-strips-100:p21.pddl', u'scanalyzer-08-strips-100:p28.pddl', u'scanalyzer-08-strips-100:p29.pddl', u'scanalyzer-08-strips-100:p30.pddl', u'scanalyzer-opt11-strips-25:p05.pddl', u'scanalyzer-opt11-strips-25:p07.pddl', u'scanalyzer-opt11-strips-25:p08.pddl', u'scanalyzer-opt11-strips-25:p09.pddl', u'scanalyzer-opt11-strips-25:p10.pddl', u'scanalyzer-opt11-strips-25:p15.pddl', u'scanalyzer-opt11-strips-25:p16.pddl', u'scanalyzer-opt11-strips-25:p17.pddl', u'scanalyzer-opt11-strips-25:p19.pddl', u'scanalyzer-opt11-strips-50:p05.pddl', u'scanalyzer-opt11-strips-50:p07.pddl', u'scanalyzer-opt11-strips-50:p08.pddl', u'scanalyzer-opt11-strips-50:p09.pddl', u'scanalyzer-opt11-strips-50:p10.pddl', u'scanalyzer-opt11-strips-50:p15.pddl', u'scanalyzer-opt11-strips-50:p16.pddl', u'scanalyzer-opt11-strips-50:p17.pddl', u'scanalyzer-opt11-strips-50:p19.pddl', u'scanalyzer-opt11-strips-75:p05.pddl', u'scanalyzer-opt11-strips-75:p07.pddl', u'scanalyzer-opt11-strips-75:p08.pddl', u'scanalyzer-opt11-strips-75:p09.pddl', u'scanalyzer-opt11-strips-75:p10.pddl', u'scanalyzer-opt11-strips-75:p15.pddl', u'scanalyzer-opt11-strips-75:p16.pddl', u'scanalyzer-opt11-strips-75:p17.pddl', u'scanalyzer-opt11-strips-75:p19.pddl', u'scanalyzer-opt11-strips-100:p05.pddl', u'scanalyzer-opt11-strips-100:p07.pddl', u'scanalyzer-opt11-strips-100:p08.pddl', u'scanalyzer-opt11-strips-100:p09.pddl', u'scanalyzer-opt11-strips-100:p10.pddl', u'scanalyzer-opt11-strips-100:p15.pddl', u'scanalyzer-opt11-strips-100:p16.pddl', u'scanalyzer-opt11-strips-100:p17.pddl', u'scanalyzer-opt11-strips-100:p19.pddl', u'tetris-opt14-strips-25:p03-8.pddl', u'tetris-opt14-strips-25:p03-10.pddl', u'tetris-opt14-strips-25:p04-10.pddl', u'tetris-opt14-strips-25:p05-8.pddl', u'tetris-opt14-strips-25:p05-10.pddl', u'tetris-opt14-strips-50:p03-8.pddl', u'tetris-opt14-strips-50:p03-10.pddl', u'tetris-opt14-strips-50:p04-10.pddl', u'tetris-opt14-strips-50:p05-8.pddl', u'tetris-opt14-strips-50:p05-10.pddl', u'tetris-opt14-strips-75:p03-8.pddl', u'tetris-opt14-strips-75:p03-10.pddl', u'tetris-opt14-strips-75:p04-10.pddl', u'tetris-opt14-strips-75:p05-8.pddl', u'tetris-opt14-strips-75:p05-10.pddl', u'tetris-opt14-strips-100:p03-8.pddl', u'tetris-opt14-strips-100:p03-10.pddl', u'tetris-opt14-strips-100:p04-10.pddl', u'tetris-opt14-strips-100:p05-8.pddl', u'tetris-opt14-strips-100:p05-10.pddl', u'transport-opt11-strips-25:p17.pddl', u'transport-opt11-strips-50:p17.pddl', u'transport-opt11-strips-75:p17.pddl', u'transport-opt11-strips-100:p17.pddl']}

SUITE = tmp_dict['ref-adapted-search-ms-lr-cb-600']
#SUITE = tmp_dict['ref-adapted-search-ms-lr-nocb-600']
 
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



config_name = 'ref-adapted-search-translator-blind-general-util-cb'
config_date = '19-10-28'
report_name = '%s-%s' % (config_name,config_date)
exppath = os.path.join('data', report_name)


exp.add_suite(BENCHMARKS_DIR, SUITE)

REV='737a1309eca1'

exp.add_algorithm('ref-adapted-search-blind-partial', REPO, REV, ['--search', "astar(blind())"])


# Add step that writes experiment files to disk.
exp.add_step('build', exp.build)

# Add step that executes all runs.
exp.add_step('start', exp.start_runs)

# Add step that collects properties from run directories and
# writes them to *-eval/properties.
exp.add_fetcher(name='fetch')
#exp.add_parse_again_step()


ATTRIBUTES = ['coverage', 'expansions', 'evaluations', 'total_time', 'util_initial_state', 'util_upper_bound', 'plan_util', 'initial_h_value']

# Make a report.
exp.add_report(
    AbsoluteReport(attributes=ATTRIBUTES),
    outfile='%s.html' % report_name)


# Parse the commandline and run the specified steps.
exp.run_steps()
