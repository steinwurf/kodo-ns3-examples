#! /usr/bin/env python
# encoding: utf-8

import os
import waflib.extras.wurf_options

APPNAME = 'kodo-ns3-examples'
VERSION = '2.0.0'


def options(opt):

    opt.load('wurf_common_tools')

    # The options needed to find the ns-3 libraries
    opt.add_option(
        '--ns3-path', default=None, dest='ns3_path',
         help='Path to ns-3')


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

    # Find the ns-3 libraries
    if not conf.options.ns3_path:
        conf.fatal('Please specify a path to ns-3 using the '
                   '--ns3-path option, for example: --ns3-path="~/ns-3-dev"')

    ns3_path = os.path.abspath(os.path.expanduser(conf.options.ns3_path))

    if not os.path.isdir(ns3_path):
        conf.fatal('The specified ns3 path "%s" is not a valid '
                   'directory' % ns3_path)

    ns3_build = os.path.join(ns3_path, 'build')
    if not os.path.isdir(ns3_build):
        conf.fatal('Could not find the ns3 build directory '
                   'in "%s"' % ns3_build)

    ns3_lib_dir = conf.root.find_dir(ns3_build)

    if conf.is_mkspec_platform('mac'):
        ns3_libs = ns3_lib_dir.ant_glob('*.dylib')
    elif conf.is_mkspec_platform('linux'):
        ns3_libs = ns3_lib_dir.ant_glob('*.so')
    else:
        conf.fatal('We could not detect your platform, please send us a '
                   'bug report')

    if not ns3_libs:
        conf.fatal('Could not find any of the ns3 shared libraries '
                   '(.so files). Please build ns3 in the given folder!')

    def get_libname(l):
        # Get the file name only
        l = os.path.basename(str(l))

        # Remove the lib prefix
        prefix = 'lib'

        if l.startswith(prefix):
            l = l[len(prefix):]

        # Remove the .so
        l = os.path.splitext(l)[0]

        return l

    ns3_lib_names = [get_libname(l) for l in ns3_libs]

    conf.env['NS3_BUILD'] = [ns3_build]
    conf.env['NS3_LIBS'] = ns3_lib_names


def build(bld):

    bld.load("wurf_common_tools")

    # The ns-3 headers produce lots of warnings with -pedantic
    if '-pedantic' in bld.env['CXXFLAGS']:
        bld.env['CXXFLAGS'].remove('-pedantic')

    if bld.is_toplevel():
        bld.recurse('src/wired_broadcast')
        bld.recurse('src/wifi_broadcast')
        bld.recurse('src/encoder_recoder_decoder')
