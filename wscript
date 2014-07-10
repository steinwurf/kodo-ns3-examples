#! /usr/bin/env python
# encoding: utf-8

import os

APPNAME = 'kodo-ns3-examples'
VERSION = '1.0.0'


def recurse_helper(ctx, name):
    if not ctx.has_dependency_path(name):
        ctx.fatal('Load a tool to find %s as system dependency' % name)
    else:
        p = ctx.dependency_path(name)
        ctx.recurse(p)

def options(opt):

    # Here we fetch Kodo and its dependencies using git
    import waflib.extras.wurf_dependency_bundle as bundle
    import waflib.extras.wurf_dependency_resolve as resolve

    # The options needed to find the ns-3 libraries
    opt.add_option(
        '--ns3-path',
        help='Install path to ns3',
        action="store", type="string", default=None,
        dest='ns3_path')

    bundle.add_dependency(opt, resolve.ResolveGitMajorVersion(
        name='boost',
        git_repository='github.com/steinwurf/external-boost-light.git',
        major_version=1))

    bundle.add_dependency(opt, resolve.ResolveGitMajorVersion(
        name='cpuid',
        git_repository='github.com/steinwurf/cpuid.git',
        major_version=3))

    bundle.add_dependency(opt, resolve.ResolveGitMajorVersion(
        name='fifi',
        git_repository='github.com/steinwurf/fifi.git',
        major_version=12))

    bundle.add_dependency(opt, resolve.ResolveGitMajorVersion(
        name='kodo',
        git_repository='github.com/steinwurf/kodo.git',
        major_version=17))

    bundle.add_dependency(opt, resolve.ResolveGitMajorVersion(
        name='platform',
        git_repository='github.com/steinwurf/platform.git',
        major_version=1))

    bundle.add_dependency(opt, resolve.ResolveGitMajorVersion(
        name='sak',
        git_repository='github.com/steinwurf/sak.git',
        major_version=11))

    bundle.add_dependency(opt, resolve.ResolveGitMajorVersion(
        name='waf-tools',
        git_repository='github.com/steinwurf/external-waf-tools.git',
        major_version=2))

    opt.load('wurf_configure_output')
    opt.load('wurf_dependency_bundle')
    opt.load('wurf_tools')


def configure(conf):

    # Configure Kodo and all its dependencies
    if conf.is_toplevel():

        conf.load('wurf_dependency_bundle')
        conf.load('wurf_tools')

        conf.load_external_tool('install_path', 'wurf_install_path')
        conf.load_external_tool('mkspec', 'wurf_cxx_mkspec_tool')
        conf.load_external_tool('project_gen', 'wurf_project_generator')
        conf.load_external_tool('runners', 'wurf_runner')

        recurse_helper(conf, 'boost')
        recurse_helper(conf, 'cpuid')
        recurse_helper(conf, 'fifi')
        recurse_helper(conf, 'kodo')
        recurse_helper(conf, 'platform')
        recurse_helper(conf, 'sak')

    # Find the ns-3 libraries
    if not conf.options.ns3_path:
        conf.fatal('Please specify a path to ns3 using the '
                   '--ns3-path option example --ns3-path=~/dev/ns3')

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
        conf.fatal('Could not find any of the ns-3 shared libraries '
                   '(.so files) are you sure you have succesfully built ns-3')

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

    for l in ns3_lib_names:

        conf.check_cxx(lib=l, libpath=[ns3_lib_dir.abspath()],
                       rpath=[ns3_lib_dir.abspath()])

    conf.env['NS3_BUILD'] = [ns3_build]
    conf.env['NS3_LIBS'] = ns3_lib_names


def build(bld):

    if bld.is_toplevel():

        bld.load('wurf_dependency_bundle')

        recurse_helper(bld, 'boost')
        recurse_helper(bld, 'cpuid')
        recurse_helper(bld, 'fifi')
        recurse_helper(bld, 'kodo')
        recurse_helper(bld, 'platform')
        recurse_helper(bld, 'sak')

    bld.recurse('wired_broadcast')
    #bld.recurse('wifi_broadcast')
