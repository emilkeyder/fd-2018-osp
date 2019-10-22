#!/usr/bin/env python

import sys, os, json

def get_domain_problem(filename):
    with open(filename, "r") as s:
        data = json.load(s)
        return data['domain'], data['problem']


def get_init_value(filename):
    with open(filename, "r") as s:
        data = json.load(s)
        if "initial_h_value" in data:
            return data["initial_h_value"]
        return None


def main(files):
    for fl in files:
        static = get_static_properties(fl)
        domain, problem = get_domain_problem(static)
        prop = get_properties(fl)
        init = get_init_value(prop)
        print("%s, %s, %s" % (domain, problem, init))

def get_properties(filename):
    dirpath = os.path.dirname(os.path.abspath(filename))
    return os.path.join(dirpath, "properties")

def get_static_properties(filename):
    dirpath = os.path.dirname(os.path.abspath(filename))
    return os.path.join(dirpath, "static-properties")

if __name__ == "__main__":
    main(sys.argv[1:])
