#! /usr/bin/env python

import sys, os, glob


def get_domain_problem(folder):
    import json
    with open(os.path.join(folder, "static-properties"), "r") as f:
        data = json.load(f)
        return data["domain"], data["problem"] #, os.path.basename(data["domain_file"])

def get_plan_util(folder):
    import json
    with open(os.path.join(folder, "properties"), "r") as f:
        data = json.load(f)
        return data["plan_util"]


def is_interesting(folder):
    return get_plan_util(folder) > 0

def get_benchmark_location():
    return '/storage/US1J6721/downward-benchmarks'

def get_orig_domain(name):
    if name.endswith('-100') or name.endswith('-125') or name.endswith('-150') or name.endswith('-175') or name.endswith('-200'):
        return name[0:-4] 
    return None

def get_domain_bound(name):
    if name.endswith('-100') or name.endswith('-125') or name.endswith('-150') or name.endswith('-175') or name.endswith('-200'):
        return name[-3:] 
    return None

def get_num_plans(folder):

    import fnmatch

    return len(fnmatch.filter(os.listdir(os.path.join(folder, 'found_plans' , 'done')), 'sas_plan.*'))
    #for infile in glob.iglob( os.path.join(folder, 'found_plans' , 'done', 'sas_plan.*') ):


def get_domain_file_name(folder):
    link = os.readlink(os.path.join(folder,"domain.pddl"))
    return os.path.basename(link)

def get_orig_problem_file_name(domain, pname):
    ## Assumption: the difference can be only in the letter case, with the exception of "pfile?.pddl" -> "p0?.pddl" and "pfile??.pddl" -> "p??.pddl"
    for f in os.listdir(domain):
        if f.lower() == pname.lower():
            return f

    if pname.startswith('pfile'):
        num = int(pname.replace('pfile', '').replace('.pddl',''))
        nname = "p%.2d.pddl" % num
        if os.path.exists(os.path.join(domain,nname)):
            return nname
    print("ERROR: %s %s " % (domain, pname))
    return pname



def get_orig_domain_file_name(dname):
    if os.path.exists(dname):
        return dname
    default = os.path.join(os.path.dirname(dname), 'domain.pddl')
    if os.path.exists(default):
        return default
    print("ERROR: %s " % dname)
    return dname

def get_orig_domain_prob(folder):
    domain, problem = get_domain_problem(folder)
    domain_file = get_domain_file_name(folder)
    orig_domain = os.path.join(get_benchmark_location(), get_orig_domain(domain))
    domain = os.path.join(orig_domain, domain_file)
    orig_problem = get_orig_problem_file_name(orig_domain, problem)
    return get_orig_domain_file_name(domain), os.path.join(orig_domain, orig_problem)


def print_attention(domain, problem, ret):
    print("ATTENTION: %s %s [%s %s %s %s %s]" % (domain, problem, ret[domain][problem]['100'], ret[domain][problem]['125'], ret[domain][problem]['150'], ret[domain][problem]['175'], ret[domain][problem]['200']))


def print_reg(domain, problem, ret):
    print("REGULAR: %s %s [%s %s %s %s %s]" % (domain, problem, ret[domain][problem]['100'], ret[domain][problem]['125'], ret[domain][problem]['150'], ret[domain][problem]['175'], ret[domain][problem]['200']))


def main(folder):
    ret = {}
    for f in glob.glob(os.path.join(folder, "runs-*", "*")):
        sas_file = os.path.join(f, 'sas_plan')
        if os.path.isfile(sas_file):
            domain, problem = get_domain_problem(f)
            bound = get_domain_bound(domain)
            plan_util = get_plan_util(f)
            if domain not in ret:
                ret[domain] = {}
            if problem not in ret[domain]:
                ret[domain][problem] = {}
            if bound not in ret[domain][problem]:
                ret[domain][problem][bound] = plan_util

    
    for domain in ret:
        for problem in ret[domain]:
            bounds = ret[domain][problem].values()
            if max(bounds) == 0 and len(bounds) == 5:
                print("NOT INTERESTING: %s %s" % (domain, problem))
            elif min(bounds) == 0:
                print_attention(domain, problem, ret)
            else:
                print_reg(domain, problem, ret)

if __name__ == "__main__":
    main(sys.argv[1])

