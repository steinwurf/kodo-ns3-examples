#! /usr/bin/env python
# encoding: utf-8

import os

from waflib.Build import BuildContext


APPNAME = "kodo-ns3-examples"
VERSION = "3.0.0"


def build(bld):

    CXX = bld.env.get_flat("CXX")

    # Matches both g++ and clang++
    if "g++" in CXX or "clang" in CXX:
        # The -fPIC flag is required for all underlying static libraries that
        # will be included in a Position Independent Executable (PIE)
        bld.env.append_value("CXXFLAGS", "-fPIC")

    # Make sure that kodo static library is compiled at the top level
    bld(
        features="cxx cxxstlib",
        name="kodo_static",
        target="kodo",
        install_path="/lib",
        use=["kodo"],
    )

    # Install all examples to the target folder
    start_dir = bld.path.find_dir("examples")
    bld.install_files(
        dest="/", files=start_dir.ant_glob("**/*"), cwd=start_dir, relative_trick=True
    )

    # Add symlinks to all dependency includes in the "include" subfolder
    projects = ["kodo", "endian"]
    for project in projects:
        subfolder = project.replace("-", "_")
        src_dir = os.path.join(bld.dependency_path(project), "src", subfolder)
        dst_dir = os.path.join("/include", subfolder)
        bld.symlink_as(dst_dir, src_dir)


class DocsContext(BuildContext):
    cmd = "docs"
    fun = "docs"


def docs(ctx):
    """Build the documentation in a virtualenv"""
    with ctx.create_virtualenv() as venv:
        venv.run(
            "python -m pip install -qq -r docs/requirements.txt",
            cwd=ctx.path.abspath(),
        )
        venv.run(
            "sphinx-build -b html -d build/doctrees docs build/html",
            cwd=ctx.path.abspath(),
        )
