#! /usr/bin/env python

import sys, json, os


def get_static(fname):
    with open(fname, 'r') as f:
        prop = json.load(f)
        dom = prop['domain']
        s = dom.split("-")[-1]
        dom2 = dom.split("-")[:-1]
        return s, "-".join(dom2), prop['problem'], prop['algorithm']


if __name__ == "__main__":
    with open(sys.argv[1], 'r') as f:
        for line in f.readlines():
            static = os.path.dirname(os.path.abspath(line))
            static = os.path.join(static, 'static-properties')
            s, _, _, alg = get_static(static)
            print("%s %s" % (s,alg))

