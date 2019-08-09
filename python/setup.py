from setuptools import setup, find_packages

from setuptools import Extension
from setuptools.command.build_ext import build_ext
import sys
import setuptools

import glob
import os

if 'build' in sys.argv:
  if not 'build_ext' in sys.argv:
    raise TypeError("Missing option 'build_ext'.")

ext_modules = [
    Extension(
        'libsunbeam',
        [
                'cxx/connection.cpp',
                'cxx/deck.cpp',
                'cxx/deck_keyword.cpp',
                'cxx/eclipse_3d_properties.cpp',
                'cxx/eclipse_config.cpp',
                'cxx/eclipse_grid.cpp',
                'cxx/eclipse_state.cpp',
                'cxx/group.cpp',
                'cxx/group_tree.cpp',
                'cxx/parser.cpp',
                'cxx/schedule.cpp',
                'cxx/sunbeam_state.cpp',
                'cxx/table_manager.cpp',
                'cxx/well.cpp',
                'cxx/sunbeam.cpp'
        ],
        libraries=['opmcommon', 'boost_filesystem', 'boost_regex', 'ecl'],
        language='c++',
        undef_macros=["NDEBUG"],
        include_dirs=["pybind11/include"]
    ),
]

setup(
    name='Sunbeam',
    package_dir = {'': 'python'},
    packages=[
                'sunbeam',
                'sunbeam.tools',
                'sunbeam.deck',
            ],
    ext_modules=ext_modules,
    license='Open Source',
    zip_safe=False,
    test_suite='tests',
    setup_requires=["pytest-runner", 'setuptools_scm'],
)
