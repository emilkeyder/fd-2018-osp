#! /usr/bin/env python

from lab.parser import Parser

"""
Solution found with utility value: 2

"""

def plan_util(content, props):
    if 'plan_util_old' in props:
        props['plan_util'] = props['plan_util_old']
    if 'plan_util_new' in props:
        props['plan_util'] = props['plan_util_new']

def osp_coverage(content, props):
    props['coverage'] = 0
    if 'plan_util_old' in props or 'plan_util_new' in props:
        props['coverage'] = 1


def por_diff(content, props):
    if 'por_before' in props and 'por_after' in props:
        props['por_diff'] = props['por_before'] - props['por_after']

parser = Parser()
parser.add_pattern('util_initial_state', r'Initial State has utility: (\d+)',type=int, required=True)
parser.add_pattern('util_upper_bound', r'Utility upper-bound: (\d+)',type=int, required=False)
parser.add_pattern('plan_util_old', r'Solution found with utility value: (\d+)',type=int, required=False)
parser.add_pattern('plan_util_new', r'Plan utility: (\d+)',type=int, required=False)

parser.add_pattern('por_before', r'total successors before partial-order reduction: (\d+)',type=int, required=False)
parser.add_pattern('por_after', r'total successors after partial-order reduction: (\d+)',type=int, required=False)

parser.add_function(plan_util)
parser.add_function(osp_coverage)
parser.add_function(por_diff)

parser.parse()
