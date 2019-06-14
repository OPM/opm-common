# pycmake

**pycmake** is a CMake macro for testing that we have the necessary Python
modules in the necessary versions for our systems.  The basic assumption of this
package is PEP 396 -- Module Version Numbers as layed out in
https://www.python.org/dev/peps/pep-0396/ .  Here it says that a Python module
*should* expose a field `__version__`, that is, to obtain a module's version,
one may run

    import module as m
    print(m.__version__)


Unfortunately, not all Python modules expose a version number, like *inspect*.
Other Python modules expose several version numbers, e.g., one for the
underlying software and one for the python packaging, like *SQLite* and *PyQt*.
To handle the different ways of obtaining the version, one may explicitly state
the version *accessor* to use; `__version__` is the default.

For more information on module version numbers, see
[PEP 396](https://www.python.org/dev/peps/pep-0396/).




## Examples

The most vanilla example usage is the following, where we require *numpy*
version at least&nbsp;1.7.0, and any newer version is acceptable.  Consider the
two CMake lines, whose behavior are identical:

    python_module( numpy REQUIRED 1.7.0         )
    python_module( numpy REQUIRED 1.7.0 MINIMUM )


However, sometimes we are phasing out an older Python module, in which case, we
can give the user a warning.  By writing

    python_module( scipy REQUIRED 1.5.1 OPTIONAL )

we are telling CMake to output a warning to the user if a *scipy* version
below&nbsp;1.5.1 is found, and to exit with an error if *scipy* is not found.

Yet other times, our systems do not work with newer versions than a certain
number.  By writing

    python_module( pandas REQUIRED 0.15.1 EXACT )

we ask CMake to fail if *pandas*&nbsp;0.15.1 is not installed, i.e., even if
*pandas*&nbsp;0.15.2 is installed.


More examples:

    find_python_module( numpy REQUIRED 1.5.3 MINIMUM               )
    find_python_module( numpy REQUIRED 1.5.3 EXACT                 )
    find_python_module( numpy REQUIRED 1.5.3 OPTIONAL              )
    find_python_module( numpy REQUIRED 1.5.3                       )
    find_python_module( numpy REQUIRED                             )
    find_python_module( numpy OPTIONAL                             )
    find_python_module( numpy REQUIRED 1.5.3 MINIMUM "__version__" )


Not every Python module exposes `__version__`, and some module exposes several
flags, like `version` and `apilevel`.

<!--
*SQLite2* users beware.  They expose `sqlite_version_info`, `version`, and
`version_info`.  There is a difference between the SQLite version (e.g.&nbsp;2
or&nbsp;3) and the *python-pysqlite* version, e.g.&nbsp;1.0.1.  *SQLite2*
exposes `apilevel = '2.0'` and `version = '1.0.1'`.  It is therefore possible to
get both `apilevel` and `version`, as well as `__version__` etc.
-->


Complex systems like SQLite and Qt exposes more than one version field.  For
instance, SQLite&nbsp;2 vs *python-pysqlite1*.  SQLite has version&nbsp;2.0,
whereas the Python package *python-pysqlite1* has version&nbsp;1.0.1.  These are
accessible by calling

    apilevel = '2.0'
    version  = '1.0.1'

It is possible to obtain these properties by invoking `python_module` providing
a fifth argument, the *version accessor* argument:

    python_module( sqlite REQUIRED 2.0   MINIMUM "apilevel"    )
    python_module( sqlite REQUIRED 1.0.1 MINIMUM "version"     )
    python_module( numpy  REQUIRED 1.7.1 MINIMUM "__version__" )


*PyQt4* does not export any version field.  Importing *PyQt4.Qt* in Python
reveals fields

    PYQT_VERSION_STR = '4.10.4'
    QT_VERSION_STR = '4.8.6'



## Technicalities

This repo contains two files of interest.  The `CMakeLists.txt` loads the more
interesting file `FindPythonModule.cmake`, and then provides some use cases.
`FindPythonModule.cmake` contains two macros, one main and one auxilliary:

* `macro( python_module_version module )`
* `macro( python_module module version )` &mdash; this is the interface

The first macro, `python_module_version`, checks if `module` is a Python
importable package, and if so, which version `module` has.  The version is found
simply by the following Python program:

    import package as py_m
    print(py_m.__version__)


If this program fails, the status flag will be set to&nbsp;`1`, and the package
will be assumed to not exist.  If the program succeeds, the output of the
program will be stored in the global cmake variable `PY_${package}` where
`${package}` is the name of the package given to the macro `python_module`.


The macro, `python_module(module version)` calls the function,
`python_module_version(module)` and checks if

1. The variable `PY_${module}` has been set.  If the package is `REQUIRED`, then
   the variable must have been set, otherwise we message a `SEND_ERROR`, and
   CMake will fail.  If the package is `OPTIONAL` and not found, we issue a
   `WARNING`.
1. The we compare the version number found (the content of the variable
   `PY_${module}`).  The keywords `EXACT`, `MINIMUM` and `OPTIONAL` are
   self-explanatory.
