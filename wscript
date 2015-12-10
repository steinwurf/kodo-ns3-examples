#! /usr/bin/env python
# encoding: utf-8

import os
import waflib.extras.wurf_options
from waflib.TaskGen import feature, before_method

APPNAME = 'kodo-ns3-examples'
VERSION = '2.0.0'


def options(opt):

    opt.load('wurf_common_tools')

    # The options needed to find the ns-3 libraries
    opt.add_option(
        '--ns3_path', default=None, dest='ns3_path',
         help='Path to the cloned ns-3 repository')


def resolve(ctx):

    import waflib.extras.wurf_dependency_resolve as resolve

    ctx.load('wurf_common_tools')

    ctx.add_dependency(resolve.ResolveVersion(
        name='waf-tools',
        git_repository='github.com/steinwurf/waf-tools.git',
        major=3))

    ctx.add_dependency(resolve.ResolveVersion(
        name='kodo-cpp',
        git_repository='github.com/steinwurf/kodo-cpp.git',
        major=4))


def configure(conf):

    conf.load("wurf_common_tools")


def build(bld):

    bld.load("wurf_common_tools")

    # Define a dummy task to force the compilation of the kodo-c shared library
    bld(features='cxx',
        use=['kodoc'])

    # Expand and validate the ns3_path option
    if not bld.has_tool_option('ns3_path'):
        conf.fatal('Please specify a path to ns-3 using the '
                   '--ns3_path option, for example: --ns3_path="~/ns-3-dev"')

    ns3_path = bld.get_tool_option('ns3_path')
    ns3_path = os.path.abspath(os.path.expanduser(ns3_path))

    if not os.path.isdir(ns3_path):
        conf.fatal('The specified ns3_path "{}" is not a valid '
                   'directory'.format(ns3_path))

    # Define the path where the kodo examples will be installed in ns-3
    bld.env['NS3_PATH'] = ns3_path
    bld.env['NS3_EXAMPLES_PATH'] = os.path.join(
        bld.env['NS3_PATH'], 'examples', 'kodo')

    # Install all files from the 'examples' folder
    start_dir = bld.path.find_dir('examples')
    bld.install_files('${NS3_EXAMPLES_PATH}',
                      start_dir.ant_glob('**/*'),
                      cwd=start_dir, relative_trick=True)

    # Install all .hpp files from kodo-cpp to the 'include' folder
    start_dir = os.path.join(bld.dependency_path('kodo-cpp'), 'src')
    start_dir = bld.root.find_dir(start_dir)
    bld.install_files('${NS3_EXAMPLES_PATH}/include',
                      start_dir.ant_glob('**/*.hpp'),
                      cwd=start_dir, relative_trick=True)

    # Install kodoc.h from kodo-c to the 'include' folder
    start_dir = os.path.join(bld.dependency_path('kodo-c'), 'src')
    start_dir = bld.root.find_dir(start_dir)
    bld.install_files('${NS3_EXAMPLES_PATH}/include',
                      ['kodoc/kodoc.h'],
                      cwd=start_dir, relative_trick=True)


@feature('cshlib', 'cxxshlib')
@before_method('apply_link')
def update_kodoc_install_path(self):
    """
    Set the install_path of the kodo-c shared library to install it
    in the '${NS3_EXAMPLES_PATH}/lib' folder
    """
    if self.name == 'kodoc':
        self.install_path = os.path.join(
            self.bld.env['NS3_EXAMPLES_PATH'], 'lib')
