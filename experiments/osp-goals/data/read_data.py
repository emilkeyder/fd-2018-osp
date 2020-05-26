#! /usr/bin/env python

import sys, os, glob


def get_domain_problem(folder):
    import json
    with open(os.path.join(folder, "static-properties"), "r") as f:
        data = json.load(f)
        return data["domain"], data["problem"] #, os.path.basename(data["domain_file"])


def get_benchmark_location():
    return '/storage/US1J6721/downward-benchmarks'

def get_orig_domain(name):
    if name.endswith('-100') or name.endswith('-125') or name.endswith('-150') or name.endswith('-175') or name.endswith('-200'):
        return name[0:-4] 
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

def validate(domain, problem, plan):
    if (sys.version_info > (3, 0)):
        import subprocess
    else:
        import subprocess32 as subprocess

    command = ["validate", domain, problem, plan]
    subprocess.check_call(command, stderr=subprocess.STDOUT)    

def main(folder):
    for f in glob.glob(os.path.join(folder, "runs-*", "*")):
        sas_file = os.path.join(f, 'sas_plan')
        if os.path.isfile(sas_file):
            domain, problem = get_orig_domain_prob(f)
            if not os.path.isfile(domain) or not os.path.isfile(problem):
                print("FILE DOES NOT EXIST")
            validate(domain, problem, sas_file)
            #print("%s, %s, %s" % (domain, problem, sas_file))

if __name__ == "__main__":
    main(sys.argv[1])

