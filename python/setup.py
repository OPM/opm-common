from setuptools import setup, find_packages

from setuptools import Extension
from setuptools.command.build_ext import build_ext
import sys
import setuptools

import glob
import os
import subprocess
try:
    from importlib.machinery import EXTENSION_SUFFIXES
    suffix = EXTENSION_SUFFIXES[0]
except ImportError:
    suffix = ".so"

setupdir = os.path.dirname(__file__)
if setupdir != '':
  os.chdir( setupdir )

try:
    subprocess.call(['ccache', '--version'])
    cc = os.environ.get("CC", "c++")
    os.environ['CC'] = 'ccache {}'.format(cc)
    print("Using 'ccache {}' as compiler".format(cc))
except OSError as e:
    print('\nNOTE: please install ccache for faster compilation of python bindings.\n')

if 'build' in sys.argv:
    if not 'build_ext' in sys.argv:
        raise TypeError("Missing option 'build_ext'.")

ext_modules = [
    Extension(
        'libopmcommon_python',
        [
                'cxx/unit_system.cpp',
                'cxx/connection.cpp',
                'cxx/converters.cpp',
                'cxx/deck.cpp',
                'cxx/deck_keyword.cpp',
                'cxx/eclipse_io.cpp',
                'cxx/field_props.cpp',
                'cxx/eclipse_config.cpp',
                'cxx/eclipse_grid.cpp',
                'cxx/eclipse_state.cpp',
                'cxx/group.cpp',
                'cxx/log.cpp',
                'cxx/parsecontext.cpp',
                'cxx/parser.cpp',
                'cxx/schedule.cpp',
                'cxx/summary_state.cpp',
                'cxx/table_manager.cpp',
                'cxx/well.cpp',
                'cxx/export.cpp'
        ],
        libraries=['opmcommon', 'boost_filesystem', 'boost_regex'],
        language='c++',
        undef_macros=["NDEBUG"],
        include_dirs=["pybind11/include"]
    ),

    Extension(
        'libopmioecl_python',
        [
                'cxx/eclipse_io.cpp',
        ],
        libraries=['opmcommon', 'boost_filesystem', 'boost_regex'],
        language='c++',
        undef_macros=["NDEBUG"],
        include_dirs=["pybind11/include"]
    )

]

setup(
    name='Opm',
    package_dir = {'': 'python'},
    packages=[
                'opm',
                'opm.io',
                'opm.io.deck',
                'opm.io.ecl_state',
                'opm.io.parser',
                'opm.io.schedule',
                'opm.io.ecl',
                'opm.tools'
            ],
    ext_modules=ext_modules,
    package_data={'opm': ['libopmcommon_python{}'.format(suffix), 'libopmioecl_python'.format(suffix)]},
    include_package_data=True,
    license='Open Source',
    zip_safe=False,
    tests_suite='tests',
    setup_requires=["pytest-runner", 'setuptools_scm'],
)
