#!/usr/bin/env python
# encoding: utf-8

import os
import sys
import json
import shutil
import subprocess

project_name = 'kodo-ns3-examples'


def run_command(args):
    print("Running: {}".format(args))
    sys.stdout.flush()
    subprocess.check_call(args)


def get_tool_options(properties):
    options = []
    if 'tool_options' in properties:
        # Make sure that the values are correctly comma separated
        for key, value in properties['tool_options'].items():
            if value is None:
                options += ['--{0}'.format(key)]
            else:
                options += ['--{0}={1}'.format(key, value)]

    return options


def configure(properties):
    # Configure this project with our waf
    command = [sys.executable, 'waf']

    if properties.get('build_distclean'):
        command += ['distclean']

    command += ['configure', '--git-protocol=git@']

    if 'waf_bundle_path' in properties:
        command += ['--bundle-path=' + properties['waf_bundle_path']]

    if 'dependency_project' in properties:
        command += ['--{0}-use-checkout={1}'.format(
            properties['dependency_project'],
            properties['dependency_checkout'])]

    command += ["--cxx_mkspec={}".format(properties['cxx_mkspec'])]
    command += get_tool_options(properties)

    run_command(command)

    ns3_path = properties['ns3_path']

    # Make sure that the previously installed examples are deleted
    examples_path = os.path.join(ns3_path, 'examples', 'kodo')
    if os.path.isdir(examples_path):
        shutil.rmtree(examples_path)

    # Clone the ns-3 repo if it is not present
    if not os.path.isdir(ns3_path):
        command = ['hg', 'clone', 'http://code.nsnam.org/ns-3-dev/', ns3_path]
        run_command(command)

    # Update ns-3-dev to the latest supported revision
    # See revisions here: http://code.nsnam.org/ns-3-dev/log
    os.chdir(ns3_path)
    run_command(['hg', 'pull'])
    run_command(['hg', 'checkout', '11767'])
    # Configure ns-3 with the examples enabled
    run_command([sys.executable, 'waf', 'configure', '--enable-examples'])


def build(properties):
    command = [sys.executable, 'waf', 'build', 'install', '-v']
    command += ['--ns3_path={}'.format(properties['ns3_path'])]
    run_command(command)


def run_tests(properties):
    command = [sys.executable, 'waf', '-v', '--run_tests']
    run_cmd = None

    if properties.get('valgrind_run'):
        run_cmd = 'valgrind --error-exitcode=1 %s'

    if run_cmd:
        command += ["--run_cmd={}".format(run_cmd)]

    command += get_tool_options(properties)

    run_command(command)


def install(properties):
    pass


def main():
    argv = sys.argv

    if len(argv) != 3:
        print("Usage: {} <command> <properties>".format(argv[0]))
        sys.exit(0)

    cmd = argv[1]
    properties = json.loads(argv[2])

    if cmd == 'configure':
        configure(properties)
    elif cmd == 'build':
        build(properties)
    elif cmd == 'run_tests':
        run_tests(properties)
    elif cmd == 'install':
        install(properties)
    else:
        print("Unknown command: {}".format(cmd))


if __name__ == '__main__':
    main()
