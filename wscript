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

#    def overwrite_symlink(from_path, to_path):
#        if os.path.lexists(path=to_path):
#            os.unlink(to_path)
#        os.symlink(from_path, to_path)

    # Add symlinks to all dependency includes under NS3_EXAMPLES_PATH/include
    if bld.cmd == 'install':
        include_dir = os.path.join(bld.env['NS3_EXAMPLES_PATH'], 'include')
        if not os.path.exists(include_dir):
            os.makedirs(include_dir)

        src_dir = os.path.join(
            bld.dependency_path('kodo-rlnc'), 'src', 'kodo_rlnc')
        dst_dir = os.path.join(include_dir, 'kodo_rlnc')
        bld.symlink_as(dst_dir, src_dir)

        src_dir = os.path.join(
            bld.dependency_path('fifi'), 'src', 'fifi')
        dst_dir = os.path.join(include_dir, 'fifi')
        bld.symlink_as(dst_dir, src_dir)

        src_dir = os.path.join(
            bld.dependency_path('storage'), 'src', 'storage')
        dst_dir = os.path.join(include_dir, 'storage')
        bld.symlink_as(dst_dir, src_dir)

        src_dir = os.path.join(
            bld.dependency_path('kodo-core'), 'src', 'kodo_core')
        dst_dir = os.path.join(include_dir, 'kodo_core')
        bld.symlink_as(dst_dir, src_dir)


@feature('cxxstlib')
@after_method('apply_incpaths')
def kodo_ns3_examples_symlink_includes(self):
    if self.name == 'kodo_rlnc':
        includes = sorted(set(self.env.INCLUDES))
        for inc in includes:
            if not inc.is_child_of(self.bld.bldnode):
                print(inc)
        #print(includes)


@feature('cxxstlib')
@before_method('apply_link')
def kodo_ns3_examples_override_stlib_install_path(self):
    """
    Install all static libraries to the NS3_EXAMPLES_PATH/lib folder
    """
    self.install_path = os.path.join(self.bld.env['NS3_EXAMPLES_PATH'], 'lib')
