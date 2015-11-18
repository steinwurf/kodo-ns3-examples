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


def build(bld):

    bld.load("wurf_common_tools")

    # The ns-3 headers produce lots of warnings with -pedantic
    if '-pedantic' in bld.env['CXXFLAGS']:
        bld.env['CXXFLAGS'].remove('-pedantic')

    # Define a dummy task to force the compilation of the kodo-c shared library
    bld(features='cxx',
        use=['kodoc'])
