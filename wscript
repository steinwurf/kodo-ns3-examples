#! /usr/bin/env python
# encoding: utf-8

import os

from waflib.TaskGen import feature, after_method, before_method

APPNAME = 'kodo-ns3-examples'
VERSION = '1.0.0'


def options(opt):

    # The options needed to find the ns-3 libraries
    opt.add_option(
        '--ns3_path', default=None, dest='ns3_path',
         help='Path to the cloned ns-3 repository')


def build(bld):

    CXX = bld.env.get_flat("CXX")

    # Matches both g++ and clang++
    if 'g++' in CXX or 'clang' in CXX:
        # The -fPIC flag is required for all underlying static libraries that
        # will be included in a Position Independent Executable (PIE)
        bld.env.append_value('CXXFLAGS', '-fPIC')

    # Define a dummy task to force the compilation of the kodo-rlnc library
    bld(features='cxx',
        use=['kodo_rlnc'])

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

    # Add symlinks to all dependency includes under NS3_EXAMPLES_PATH/include
    include_dir = os.path.join(bld.env['NS3_EXAMPLES_PATH'], 'include')

    projects = ['kodo-rlnc', 'kodo-core', 'fifi', 'storage']
    for project in projects:
        subfolder = project.replace('-', '_')
        src_dir = os.path.join(
            bld.dependency_path(project), 'src', subfolder)
        dst_dir = os.path.join(include_dir, subfolder)
        bld.symlink_as(dst_dir, src_dir)

    # Boost is added separately, since it has a different include path
    src_dir = os.path.join(
        bld.dependency_path('boost'), 'boost')
    dst_dir = os.path.join(include_dir, 'boost')
    bld.symlink_as(dst_dir, src_dir)


@feature('cxxstlib')
@before_method('apply_link')
def kodo_ns3_examples_override_stlib_install_path(self):
    """
    Install all static libraries to the NS3_EXAMPLES_PATH/lib folder
    """
    self.install_path = os.path.join(self.bld.env['NS3_EXAMPLES_PATH'], 'lib')
