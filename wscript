#! /usr/bin/env python
# encoding: utf-8

import os

from waflib import Options
from waflib.Build import BuildContext


APPNAME = 'kodo-ns3-examples'
VERSION = '3.0.0'


def options(opt):

    opt.add_option(
        '--all_docs', default=False, action='store_true',
        help='Generate all documentation versions using giit.')


def build(bld):

    CXX = bld.env.get_flat("CXX")

    # Matches both g++ and clang++
    if 'g++' in CXX or 'clang' in CXX:
        # The -fPIC flag is required for all underlying static libraries that
        # will be included in a Position Independent Executable (PIE)
        bld.env.append_value('CXXFLAGS', '-fPIC')

    # Make sure that kodo-rlnc static library is compiled at the top level
    bld(features='cxx cxxstlib',
        name='kodo_rlnc_static',
        target='kodo_rlnc',
        install_path='/lib',
        use=['kodo_rlnc'])

    # Install all examples to the target folder
    start_dir = bld.path.find_dir('examples')
    bld.install_files(
        dest = '/',
        files = start_dir.ant_glob('**/*'),
        cwd=start_dir,
        relative_trick=True)

    # Add symlinks to all dependency includes in the "include" subfolder
    projects = ['kodo-rlnc', 'kodo-core', 'fifi', 'storage', 'endian']
    for project in projects:
        subfolder = project.replace('-', '_')
        src_dir = os.path.join(
            bld.dependency_path(project), 'src', subfolder)
        dst_dir = os.path.join("/include", subfolder)
        bld.symlink_as(dst_dir, src_dir)

    # Boost is added separately, since it has a different include path
    src_dir = os.path.join(bld.dependency_path('boost'), 'boost')
    bld.symlink_as("/include/boost", src_dir)


class DocsContext(BuildContext):
    cmd = 'docs'
    fun = 'docs'


def docs(ctx):
    """ Build the documentation in a virtualenv """
    with ctx.create_virtualenv(cwd=ctx.bldnode.abspath()) as venv:
        if not ctx.options.all_docs:
            venv.run('python -m pip install -qq -r docs/requirements.txt',
                     cwd=ctx.path.abspath())
            venv.run('sphinx-build -b html -d build/doctrees docs build/html',
                     cwd=ctx.path.abspath())
        else:
            giit = 'git+https://github.com/steinwurf/giit.git@master'
            venv.pip_install(packages=[giit])
            build_path = os.path.join(ctx.path.abspath(), 'build', 'giit')
            venv.run('giit clean . --build_path {}'.format(build_path),
                     cwd=ctx.path.abspath())
            venv.run('giit sphinx . --build_path {}'.format(build_path),
                     cwd=ctx.path.abspath())
            venv.run('giit versjon . --build_path {}'.format(build_path),
                     cwd=ctx.path.abspath())