#  Copyright (C)  2016 Statoil ASA, Norway.
#
#  pymake is free software: you can redistribute it and/or modify it under the
#  terms of the GNU General Public License as published by the Free Software
#  Foundation, either version 3 of the License, or (at your option) any later
#  version.
#
#  pymake is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
#  A PARTICULAR PURPOSE.
#
#  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
#  for more details

# This module exports the following functions:
# * python_library(<target> [REQUIRE_LIBS] [EXACT] [VERSION <ver>]
#   Create a new interface library <target> that works well with
#   target_link_libraries. If the python interpreter and/or libraries are not
#   found before this function is called, it will find and configure them for you.
#
#   When the REQUIRE_LIBS option is used, this function will fail if the python
#   libraries cannot be found. Otherwise the libraries are considered optional.
#
#   When EXACT is specified and VERSION is not used, this option is ignored.
#   When exact is used with a version string, this function will fail in case a
#   different python version than <ver> is found. Python the X.Y and X.Y.Z
#   formats are supported.
#
#   When VERSION <ver> is used (without exact), this specifies the minimum
#   python version.
#
#   This function requires cmake3
#
# * add_python_package(<target> <name>
#                      [APPEND] [VERSION__INIT__]
#                      [TARGET_COPYONLY] [NO_LINK_FLAGS]
#                      [SUBDIR <dir>] [PATH <path>]
#                      [VERSION <version>]
#                      [TARGETS <tgt>...] [SOURCES <src>...]
#                      [DEPEND_DIRS <tgt1> <dir1> [<tgt2> <dir2>]...]
#   Create a new target <target>, analogous to add_library. Creates a python
#   package <name>, optionally at the path specified with PATH. If SUBDIR <dir>
#   is used, then the files will be copied to $root/dir/*, in order to create
#   python sub namespaces - if subdir is not used then all files will be put in
#   $root/*. SOURCES <src> is the python files to be copied to the directory in
#   question, and <tgt> are regular cmake libraries (targets created by
#   add_library).
#
#   When the APPEND option is used, the files and targets given will be added
#   onto the same target package - it is necessary to use APPEND when you want
#   sub modules. Consider the package foo, with the sub module bar. In python,
#   you'd do: `from foo.bar import baz`. This means the directory structure is
#   `foo/bar/baz.py` in the package. This is accomplished with:
#   add_python_package(mypackage mypackage SOURCES __init__.py)
#   add_python_package(mypackage mypackage APPEND SOURCES baz.py)
#
#   When VERSION__INIT__ is used, the pycmake will inject __version__ = '$ver'
#   in the __init__.py file. This version is read from PROJECT_VERSION unless
#   VERSION argument is used. If VERSION is used, this version is used instead.
#   If neither PROJECT_VERSION or VERSION is used, the string "0.0.0" is used
#   as a fallback. The same version number will be used for the add_setup_py
#   pip package.
#
#   Without TARGET_COPYONLY, add_python_package will by default assume that the
#   targets are python extension added in the same CMakeLists.txt file that
#   invokes the function, and will modify some output properties of the targets
#   accordingly. Some python modules are meant to be used with raw dlopen and
#   only copied as-is into the python directory, and this option is intended
#   for such packages. TARGET_COPYONLY implies NO_LINK_FLAGS
#
#   NO_LINK_FLAGS stops add_python_package from adding linker flags such as
#   export-dynamic, which is often expected by python extensions to keep them
#   independent of a very specific interpreter+library version.
#
#   DEPEND_DIRS is needed by add_setup_py if sources for the target is set with
#   relative paths. These paths can be set later in order to be less intrusive
#   on non-python aspects of the cmake file. Still, this information is
#   necessary to accurately find and move source files to the build directory,
#   so that setup.py can find them, and might need to be added later.
#
#   To override the version number used for this package, pass the VERSION
#   argument with a complete string. If this option is not used and
#   PROJECT_VERSION is set (CMake 3.x+), PROJECT_VERSION is used.
#
#   This command provides install targets, but no exports.
#
# * add_setup_py(<target> <template>
#                [MANIFEST <manifest>]
#                [OUTPUT <output>])
#
#   Create a setuptools package that is capable of building (for sdist+bdist)
#   and uploading packages to pypi and similar.
#
#   The target *must* be a target created by add_python_package. The template
#   is any setup.py that works with cmake's configure_file.
#
#   A manifest will be created and project-provided header files will be
#   included, suitable for source distribution. If you want to include other
#   things in the package that isn't suitable to add to the setup.py template,
#   point the MANIFEST argument to your base file.
#
#  This command outputs setup.py by default, but if OUTPUT is specified, the
#  generated file is <output> instead
#
# * add_python_test(testname python_test_file)
#       which sets up a test target (using pycmake_test_runner.py, distributed
#       with this module) and registeres it with ctest.
#
# * add_python_example(package example testname test_file [args...])
#       which sets up an example program which will be run with the arguments
#       [args...] (can be empty) Useful to make sure some program runs
#       correctly with the given arguments, and which will report as a unit
#       test failure.

include(CMakeParseArguments)

function(pycmake_to_path_list var path1)
    if("${CMAKE_HOST_SYSTEM}" MATCHES ".*Windows.*")
        set(sep "\\;")
    else()
        set(sep ":")
    endif()
    set(result "${path1}") # First element doesn't require separator at all...
    foreach(path ${ARGN})
        set(result "${result}${sep}${path}") # .. but other elements do.
    endforeach()
    set(${var} "${result}" PARENT_SCOPE)
endfunction()

function(pycmake_init)
    if (NOT PYTHON_EXECUTABLE OR NOT PYTHON_VERSION_STRING)
        find_package(PythonInterp REQUIRED)

        set(PYTHON_VERSION_MAJOR  ${PYTHON_VERSION_MAJOR}  CACHE INTERNAL "")
        set(PYTHON_VERSION_MINOR  ${PYTHON_VERSION_MINOR}  CACHE INTERNAL "")
        set(PYTHON_VERSION_STRING ${PYTHON_VERSION_STRING} CACHE INTERNAL "")
        set(PYTHON_EXECUTABLE     ${PYTHON_EXECUTABLE}     CACHE INTERNAL "")
    endif ()

    set(pyver ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})

    if (EXISTS "/etc/debian_version")
        set(PYTHON_PACKAGE_PATH "dist-packages")
    else()
        set(PYTHON_PACKAGE_PATH "site-packages")
    endif()

    set(PYTHON_INSTALL_PREFIX "lib/python${pyver}/${PYTHON_PACKAGE_PATH}"
        CACHE STRING "Subdirectory to install Python modules in")
endfunction()

function(pycmake_list_concat out)
    foreach (arg ${ARGN})
        list(APPEND l ${arg})
    endforeach ()

    set(${out} ${l} PARENT_SCOPE)
endfunction ()

function(pycmake_is_system_path out path)
    string(FIND ${path} ${CMAKE_SOURCE_DIR} ${out})

    if (${out} EQUAL -1)
        set(${out} TRUE PARENT_SCOPE)
    else ()
        set(${out} FALSE PARENT_SCOPE)
    endif ()
endfunction ()

# internal. Traverse the tree of dependencies (linked targets) that are actual
# cmake targets and add to a list
function(pycmake_target_dependencies dependencies links target)
    get_target_property(deps ${target} INTERFACE_LINK_LIBRARIES)

    if (NOT deps)
        get_target_property(deps ${target} LINK_LIBRARIES)
    endif ()

    # LINK_LIBRARIES could be not-found, in which we make it an empty list
    if (NOT deps)
        set(deps "")
    endif ()

    set(_links "")
    list(APPEND _dependencies ${target})
    foreach (dep ${deps})
        if (TARGET ${dep})
            pycmake_target_dependencies(trans_tgt trans_link ${dep})
            foreach (link ${trans_tgt})
                list(APPEND _dependencies ${link})
            endforeach ()
            pycmake_list_concat(_links ${_links} ${trans_link})
        else ()
            list(APPEND _links ${dep})
        endif ()
    endforeach ()

    if ( _dependencies )
        list(REMOVE_DUPLICATES _dependencies)
    endif ()
    if (_links)
        list(REMOVE_DUPLICATES _links)
    endif ()
    set(${dependencies} "${_dependencies}" PARENT_SCOPE)
    set(${links} "${_links}" PARENT_SCOPE)
endfunction ()

# internal. Traverse the set of dependencies (linked targets) to some parent
# and create a list of its source files, preprocessor definitions, include
# directories and custom compiler options, and write these as properties on the
# the target.
#
# In effect, these properties are set on the python package target (created
# with add_python_package):
#
# PYCMAKE_EXTENSIONS - a list of extensions (C/C++ targets) for the package
# For each extension in this list, these variables are set on the package:
# PYCMAKE_<ext>_INCLUDE_DIRECTORIES
# PYCMAKE_<ext>_SOURCES
# PYCMAKE_<ext>_COMPILE_DEFINITIONS
# PYCMAKE_<ext>_COMPILE_OPTIONS
# PYCMAKE_<ext>_LINK_LIBRARIES
#
# All properties are lists, and the content correspond to the non-namespaced
# properties (includes, sources etc.)
function(pycmake_include_target_deps pkg tgt depend_dirs)
    pycmake_target_dependencies(deps links ${tgt})
    set(includes "")
    set(defines "")
    set(sources "")
    set(flags "")
    if (NOT links)
        set(links "")
    endif ()

    foreach (dep ${deps})
        # If this is an interface library then most of these are probably empty
        # *and* cmake will crash if we look up any non-INTERFACE_ properties,
        # so prepend INTERFACE_ on interface targets
        unset(INTERFACE_)
        get_target_property(type ${dep} TYPE)
        if (type STREQUAL INTERFACE_LIBRARY)
            set(INTERFACE_ INTERFACE_)
        endif ()

        get_target_property(incdir ${dep} ${INTERFACE_}INCLUDE_DIRECTORIES)
        get_target_property(srcs   ${dep} ${INTERFACE_}SOURCES)
        get_target_property(defs   ${dep} ${INTERFACE_}COMPILE_DEFINITIONS)
        get_target_property(flgs   ${dep} ${INTERFACE_}COMPILE_OPTIONS)

        # prune -NOTFOUND props
        foreach (var incdir srcs defs flgs)
            if(NOT ${var})
                set(${var} "")
            endif ()
        endforeach ()

        # If sources files were registered with absolute path (prefix'd with
        # ${CMAKE_SOURCE_DIR}) we can just use this absolute path and
        # be fine. If not, we assume that if the source file is *not* relative
        # but below the current dir if it's NOT in the depend_dir list, in
        # which case we make it absolute. This ends up in the sources argument
        # to Extensions in setup.py
        list(FIND depend_dirs ${dep} index)
        if (NOT ${index} EQUAL -1)
            math(EXPR index "${index} + 1")
            list(GET depend_dirs ${index} prefix)
        else ()
            set(prefix ${CMAKE_CURRENT_SOURCE_DIR})
        endif ()

        unset(_srcs)
        foreach (src ${srcs})
            string(FIND ${src} ${CMAKE_SOURCE_DIR} x)
            if(${x} EQUAL 0)
                list(APPEND _srcs ${src})
            else()
                list(APPEND _srcs ${prefix}/${src})
            endif()
        endforeach ()
        unset(prefix)

        list(APPEND includes ${incdir})
        list(APPEND sources  ${_srcs})
        list(APPEND defines  ${defs})
        list(APPEND flags    ${flgs})
    endforeach()

    get_target_property(extensions ${pkg} PYCMAKE_EXTENSIONS)
    list(APPEND extensions ${tgt})

    # properties may contain generator expressions, which we filter out

    string(REGEX REPLACE "\\$<[^>]+>;?" "" includes "${includes}")
    string(REGEX REPLACE "\\$<[^>]+>;?" "" sources  "${sources}")
    string(REGEX REPLACE "\\$<[^>]+>;?" "" defines  "${defines}")
    string(REGEX REPLACE "\\$<[^>]+>;?" "" flags    "${flags}")

    # sources (on shared windows build) can contain .def files for exporting
    # symbols. These are filtered out too, as exporting non-python symbols is
    # uninteresting from a pip setting.
    string(REGEX REPLACE "[^;]*[.]def;?" "" sources "${sources}")

    set_target_properties(${pkg} PROPERTIES
                            PYCMAKE_EXTENSIONS "${extensions}"
                            PYCMAKE_${tgt}_INCLUDE_DIRECTORIES "${includes}"
                            PYCMAKE_${tgt}_SOURCES "${sources}"
                            PYCMAKE_${tgt}_LINK_LIBRARIES "${links}"
                            PYCMAKE_${tgt}_COMPILE_DEFINITIONS "${defines}"
                            PYCMAKE_${tgt}_COMPILE_OPTIONS "${flags}")
endfunction()

function(python_library target)
    pycmake_init()

    set(options EXACT REQUIRE_LIBS)
    set(unary VERSION)
    set(nary)
    cmake_parse_arguments(PP "${options}" "${unary}" "${nary}" "${ARGN}")

    if (PP_VERSION)
        string(REGEX MATCH "[0-9][.][0-9]+" xy "${PP_VERSION}")
        if (NOT xy)
            set(problem "Unexpected format for VERSION argument")
            set(solution "Was ${PP_VERSION}, expected X.Y (e.g. 3.5)")
            message(SEND_ERROR "${problem}. ${solution}")
        endif ()
    endif ()

    if (PP_EXACT AND NOT PP_VERSION)
        unset(PP_EXACT)
    endif ()

    set(xyver ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})
    if (PP_EXACT AND ("${PYTHON_VERSION_STRING}" VERSION_EQUAL "${PP_VERSION}"
                    OR ${xyver} VERSION_EQUAL "${PP_VERSION}"))
        set(problem "Wrong Python version")
        set(solution "${PP_VERSION} requested, got ${PYTHON_VERSION_STRING}")
        message(SEND_ERROR "${problem}: ${solution}.")
    endif ()

    if (PP_VERSION AND "${PYTHON_VERSION_STRING}" VERSION_LESS "${PP_VERSION}")
        set(found "Found Python ${PYTHON_VERSION_STRING}")
        set(requested "Python >= ${PP_VERSION} requested")
        message(SEND_ERROR "${found}, but ${requested}.")
    endif ()

    add_library(${target} INTERFACE)

    if (PP_REQUIRE_LIBS)
        set(req REQUIRED)
    endif ()

    if (NOT PYTHONLIBS_FOUND)
        find_package(PythonLibs ${req})

        if (NOT PYTHONLIBS_FOUND)
            return ()
        endif ()

        set(PYTHONLIBS_FOUND    ${PYTHONLIBS_FOUND}    CACHE INTERNAL "")
        set(PYTHON_LIBRARIES    ${PYTHON_LIBRARIES}    CACHE INTERNAL "")
        set(PYTHON_INCLUDE_DIRS ${PYTHON_INCLUDE_DIRS} CACHE INTERNAL "")
    endif ()

    target_link_libraries(${target} INTERFACE ${PYTHON_LIBRARIES})
    target_include_directories(${target} SYSTEM INTERFACE ${PYTHON_INCLUDE_DIRS})
endfunction()

function(add_python_package pkg NAME)
    set(options APPEND VERSION__INIT__ TARGET_COPYONLY NO_LINK_FLAGS)
    set(unary PATH SUBDIR VERSION)
    set(nary  TARGETS SOURCES DEPEND_DIRS)
    cmake_parse_arguments(PP "${options}" "${unary}" "${nary}" "${ARGN}")

    if (TARGET_COPYONLY)
        set(PP_NO_LINK_FLAGS TRUE)
    endif ()

    pycmake_init()
    set(installpath ${CMAKE_INSTALL_PREFIX}/${PYTHON_INSTALL_PREFIX})

    if (PP_PATH)
        # obey an optional path to install into - but prefer the reasonable
        # default of currentdir/name
        set(dstpath ${PP_PATH})
    else ()
        set(dstpath ${NAME})
    endif()

    # if APPEND is passed, we *add* files/directories instead of creating it.
    # this can be used to add sub directories to a package. If append is passed
    # and the target does not exist - create it
    if (TARGET ${pkg} AND NOT PP_APPEND)
        set(problem "Target '${pkg}' already exists")
        set(descr "To add more files to this package")
        set(solution "${descr}, use add_python_package(<target> <name> APPEND)")
        message(SEND_ERROR "${problem}. ${solution}.")

    elseif (NOT TARGET ${pkg})
        add_custom_target(${pkg} ALL)

        get_filename_component(abspath ${CMAKE_CURRENT_BINARY_DIR} ABSOLUTE)
        set_target_properties(${pkg} PROPERTIES PACKAGE_INSTALL_PATH ${installpath})
        set_target_properties(${pkg} PROPERTIES PACKAGE_BUILD_PATH   ${abspath})
        set_target_properties(${pkg} PROPERTIES PYCMAKE_PACKAGE_NAME ${NAME})
        set_target_properties(${pkg} PROPERTIES PYCMAKE_PACKAGES     ${NAME})

        set(pkgver "0.0.0")
        if (PROJECT_VERSION)
            set(pkgver ${PROJECT_VERSION})
        endif ()

        if (PP_VERSION)
            set(pkgver ${PP_VERSION})
        endif ()

        set_target_properties(${pkg} PROPERTIES PYCMAKE_PACKAGE_VERSION ${pkgver})

        # set other properties we might populate later
        set_target_properties(${pkg} PROPERTIES PYCMAKE_EXTENSIONS "")

    endif ()
    # append subdir if requested
    if (PP_SUBDIR)
        set(dstpath ${dstpath}/${PP_SUBDIR})

        # save modules added with SUBDIR - setup.py will want them in packages
        get_target_property(_packages ${pkg} PYCMAKE_PACKAGES)
        get_target_property(_pkgname  ${pkg} PYCMAKE_PACKAGE_NAME)
        list(APPEND _packages ${_pkgname}/${PP_SUBDIR})
        set_target_properties(${pkg} PROPERTIES PYCMAKE_PACKAGES "${_packages}")
        unset(_packages)
        unset(_pkgname)
    endif ()

    # this is pretty gritty, but cmake has no generate-time file append write a
    # tiny cmake script that appends to some file and writes the version
    # string, which hooks into add_custom_command, and append this command on
    # the copy of the __init__ requested for versioning. This means writing
    # __version__ is a part of the file copy itself and won't be considered a
    # change to the file.
    if (PP_VERSION__INIT__)
        if (PP_SUBDIR)
            set(f ${PP_SUBDIR})
        else ()
            set(f init)
        endif ()
        string(REGEX REPLACE "[:/\\]" "-" initscript "${pkg}.${f}.cmake")
        unset(f)
        set(initscript ${CMAKE_CURRENT_BINARY_DIR}/${initscript})

        message(STATUS "Writing to " ${initscript})
        file(WRITE ${initscript}
            "file(APPEND \${PYCMAKE__INIT__} __version__='${pkgver}')"
        )
    endif ()

    # copy all .py files into
    foreach (file ${PP_SOURCES})

        get_filename_component(absfile ${file} ABSOLUTE)
        get_filename_component(fname ${file} NAME)

        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${dstpath})
        add_custom_command(OUTPUT ${dstpath}/${fname}
            COMMAND ${CMAKE_COMMAND} -E copy ${absfile} ${dstpath}/
            DEPENDS ${absfile}
        )

        list(APPEND _files ${CMAKE_CURRENT_BINARY_DIR}/${dstpath}/${fname})

        if ("${fname}" STREQUAL "__init__.py" AND PP_VERSION__INIT__)
            message(STATUS "Writing __version__ ${pkgver} to package ${pkg}.")
            add_custom_command(OUTPUT ${dstpath}/${fname}
                COMMAND ${CMAKE_COMMAND} -DPYCMAKE__INIT__=${dstpath}/${fname}
                                         -P ${initscript}
                APPEND
            )
            unset(initpy)
            unset(initscript)
        endif ()
    endforeach ()

    # drive the copying of .py files and add the dependency on the python
    # package target
    get_target_property(_pkgname  ${pkg} PYCMAKE_PACKAGE_NAME)
    set(_id ${pkg}-${_pkgname})
    string(REGEX REPLACE "[:/\\]" "-" _subdir "${PP_SUBDIR}-${_pkgname}")

    # make target-names slightly nicer, i.e. use subdir as target names
    if (NOT TARGET ${pkg}-${_subdir} AND PP_SUBDIR)
        set(pycmake-${pkg}-${_subdir} 0 CACHE INTERNAL "")
        set(_id ${pkg}-${_subdir})
    elseif (PP_SUBDIR)
        # The same SUBDIR has been used multiple times for this target Since
        # it's not possible to append source files to custom targets, a new one
        # is created with an enumerator, incremented for every extra use.
        math(EXPR _id "${pycmake-${pkg}-${_subdir}} + 1")
        set(pycmake-${pkg}-${_subdir} ${_id} CACHE INTERNAL "" FORCE)
        set(_id ${pkg}-${_subdir}-${_id})
    endif ()

    add_custom_target(${_id} ALL SOURCES ${_files} DEPENDS ${_files})
    add_dependencies(${pkg} ${_id})
    unset(_id)
    unset(_subdir)

    # targets are compiled as regular C/C++ libraries (via add_library), before
    # we add some python specific stuff for the linker here.
    set(SUFFIX ".so")
    if (WIN32 OR CYGWIN)
        # on windows, .pyd is used as extension instead of DLL
        set(SUFFIX ".pyd")
    elseif (APPLE)
        # the spaces in LINK_FLAGS are important; otherwise APPEND_STRING to
        # set_property seem to combine it with previously-set options or
        # mangles it in some other way
        set(LINK_FLAGS " -undefined dynamic_lookup ")
    else()
        set(LINK_FLAGS " -Xlinker -export-dynamic ")
    endif()

    # register all targets as python extensions
    foreach (tgt ${PP_TARGETS})
        if (LINK_FLAGS AND NOT PP_NO_LINK_FLAGS)
            set_property(TARGET ${tgt} APPEND_STRING PROPERTY LINK_FLAGS ${LINK_FLAGS})
        endif()

        if (PP_TARGET_COPYONLY)
            # ecl and other libraries relies on ctypes and dlopen. We want to
            # copy a proper install'd target when we invoke make install, in
            # particular because cmake then handles rpath stripping properly,
            # so we do a dummy install into our build directory and then
            # immediately install that. This does not modify the target in any
            # way and does not require it to be in the same directory.
            #
            # for the build tree, the library is simply copied

            # copy all targets into the package directory
            get_target_property(_lib ${tgt} OUTPUT_NAME)
            if (NOT _lib)
                # if OUTPUT_NAME is not set, library base name is the same as the
                # target name
                set(_lib ${tgt})
            endif ()

            string(REGEX REPLACE "^lib" "" _lib ${_lib}${SUFFIX})
            file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${dstpath})

            add_custom_command(OUTPUT ${dstpath}/${_lib}
                COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${tgt}> ${dstpath}/${_lib}
                DEPENDS ${tgt}
            )

            # Install the shared library as-is. Note that cmake typically
            # strips build-time rpath on install, but this object is in fact
            # never installed, so it possibly keeps rpaths to the build directory
            install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${dstpath}/${_lib}
                    DESTINATION ${installpath}/${dstpath}
            )

            add_custom_target(pycmake-ext-${pkg}-${_lib} ALL DEPENDS ${dstpath}/${_lib})
            add_dependencies(${pkg} pycmake-ext-${pkg}-${_lib})
            unset(_lib)
        endif()

        # traverse all dependencies and get their include dirs, link flags etc.
        pycmake_include_target_deps(${pkg} ${tgt} "${PP_DEPEND_DIRS}")

    endforeach ()

    if (NOT PP_TARGET_COPYONLY AND PP_TARGETS)
        install(TARGETS ${PP_TARGETS}
                LIBRARY DESTINATION ${installpath}/${dstpath}
        )

        # proper python extensions - they're assumed to be created in the
        # same dir as add_python_package is invoked and directly modify the
        # target by changing output dir and setting suffix.
        #
        # The python package does not distinguish between debug/release
        # builds, but this usually only matters for Windows.
        #
        # LIBRARY_OUTPUT_DIRECTORY only works with cmake3.0 - to support cmake
        # 2.8.12, also copy the file.
        set_target_properties(${PP_TARGETS}  PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY         ${dstpath}
            LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${dstpath}
            LIBRARY_OUTPUT_DIRECTORY_RELEASE ${dstpath}
            RUNTIME_OUTPUT_DIRECTORY         ${dstpath}
            RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${dstpath}
            RUNTIME_OUTPUT_DIRECTORY_RELEASE ${dstpath}
            PREFIX ""
            SUFFIX "${SUFFIX}"
        )

        if (${CMAKE_VERSION} VERSION_LESS 3.0)
            foreach(tgt ${PP_TARGETS})
                add_custom_command(TARGET ${tgt}
                    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${tgt}> ${dstpath}/
                )
            endforeach()
        endif ()
    endif ()

    if (_files)
        install(FILES ${_files} DESTINATION ${installpath}/${dstpath})
        unset(_files)
    endif ()

    if (NOT PP_SOURCES AND NOT PP_TARGETS AND NOT PP_APPEND)
        message(SEND_ERROR
            "add_python_package called without .py files or C/C++ targets.")
    endif()
endfunction()

function(add_setup_py target template)
    set(options)
    set(unary MANIFEST OUTPUT)
    set(nary)
    cmake_parse_arguments(PP "${options}" "${unary}" "${nary}" "${ARGN}")

    string(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype)

    get_target_property(PYCMAKE_PACKAGE_NAME ${target} PYCMAKE_PACKAGE_NAME)
    get_target_property(PYCMAKE_VERSION ${target} PYCMAKE_PACKAGE_VERSION)
    get_target_property(extensions ${target} PYCMAKE_EXTENSIONS)
    get_target_property(pkg ${target} PYCMAKE_PACKAGES)

    get_directory_property(dir_inc INCLUDE_DIRECTORIES)
    get_directory_property(dir_def COMPILE_DEFINITIONS)
    get_directory_property(dir_opt COMPILE_OPTIONS)
    string(REGEX REPLACE " " ";" dir_opt "${dir_opt}")

    set(cflags   ${CMAKE_C_FLAGS})
    set(cxxflags ${CMAKE_CXX_FLAGS})
    if (buildtype)
        set(cflags   ${cflags}   ${CMAKE_C_FLAGS_${buildtype}})
        set(cxxflags ${cxxflags} ${CMAKE_CXX_FLAGS_${buildtype}})
    endif ()

    string(REGEX REPLACE " " ";" cflags "${cflags}")
    string(REGEX REPLACE " " ";" cxxflags "${cxxflags}")
    set(flags ${cflags} ${cxxflags})

    foreach (item ${pkg})
        string(REGEX REPLACE "/" "." item ${item})
        list(APPEND _pkg "'${item}'")
    endforeach()
    string(REGEX REPLACE ";" "," pkg "${_pkg}")
    set(PYCMAKE_PACKAGES "${pkg}")

    foreach (ext ${extensions})

        get_target_property(inc ${target} PYCMAKE_${ext}_INCLUDE_DIRECTORIES)
        get_target_property(src ${target} PYCMAKE_${ext}_SOURCES)
        get_target_property(lnk ${target} PYCMAKE_${ext}_LINK_LIBRARIES)
        get_target_property(def ${target} PYCMAKE_${ext}_COMPILE_DEFINITIONS)
        get_target_property(opt ${target} PYCMAKE_${ext}_COMPILE_OPTIONS)

        pycmake_list_concat(inc ${dir_inc} ${inc})
        pycmake_list_concat(def ${dir_def} ${def})
        pycmake_list_concat(opt ${flags} ${dir_opt} ${opt})

        # remove the python include dir and lib (which is obviously unecessary)

        if(inc AND PYTHON_INCLUDE_DIRS)
            list(REMOVE_ITEM inc ${PYTHON_INCLUDE_DIRS})
        endif()
        if(lnk AND PYTHON_LIBRARIES)
            list(REMOVE_ITEM lnk ${PYTHON_LIBRARIES})
        endif()

        # wrap every string in single quotes (because python expects this)
        foreach (item ${inc})
            # project-provided headers must be bundled for sdist, so add them
            # to the pycmake/include directory
            get_filename_component(dstpath pycmake/include/${item} DIRECTORY)
            string(REGEX REPLACE "${CMAKE_SOURCE_DIR}" "" dstpath "${dstpath}")
            string(REGEX REPLACE "${CMAKE_BINARY_DIR}" "" dstpath "${dstpath}")
            string(REGEX REPLACE "//" "/" dstpath "${dstpath}")

            pycmake_is_system_path(syspath ${item})
            if (NOT ${syspath})
                file(COPY ${item} DESTINATION ${dstpath})
            endif ()
            get_filename_component(item ${item} NAME)
            list(APPEND _inc "'${dstpath}/${item}'")
            unset(dstpath)
        endforeach ()

        foreach (item ${src})
            # setup.py is pretty grumpy and wants source files relative itself
            # AND not upwards, so we must copy our entire source tree into the
            # build dir
            string(REGEX REPLACE "${CMAKE_SOURCE_DIR}" "" dstitem "pycmake/src/${item}")
            string(REGEX REPLACE "${CMAKE_BINARY_DIR}" "" dstitem "${dstitem}")
            string(REGEX REPLACE "//" "/" dstitem "${dstitem}")
            configure_file(${item} ${CMAKE_CURRENT_BINARY_DIR}/${dstitem} COPYONLY)
            list(APPEND _src "'${dstitem}'")
            unset(dstitem)
        endforeach ()

        foreach (item ${opt})
            list(APPEND _opt "'${item}'")
        endforeach ()

        set(_lnk "")
        foreach (item ${lnk})
            get_filename_component(_libdir ${item} DIRECTORY)

            if (_libdir)
                list(APPEND _dir '${_libdir}')
                get_filename_component(item ${item} NAME_WE)
                string(REGEX REPLACE "lib" "" item ${item})
            endif ()

            list(APPEND _lnk "'${item}'")
        endforeach ()

        # defines are a bit more work, because setup.py expects them as tuples
        foreach (item ${def})
            string(FIND ${item} "=" pos)

            set(_val None)
            string(SUBSTRING "${item}" 0 ${pos} _name)

            if (NOT ${pos} EQUAL -1)
                math(EXPR pos "${pos} + 1")
                string(SUBSTRING "${item}" ${pos} -1 _val)
                set(_val '${_val}')
            endif ()

            list(APPEND _def "('${_name}', ${_val})")
        endforeach ()

        if (_inc)
            list(REMOVE_DUPLICATES _inc)
        endif ()
        if( _src)
            list(REMOVE_DUPLICATES _src)
        endif ()
        if( _def)
            list(REMOVE_DUPLICATES _def)
        endif ()
        if( _lnk)
            list(REMOVE_DUPLICATES _lnk)
        endif ()
        # do not remote duplictes for compiler options, because some are
        # legitemately passed multiple times, e.g. on clang for osx builds
        # `-arch i386 -arch x86_64`

        # then make the list comma-separated (for python)
        string(REGEX REPLACE ";" "," inc "${_inc}")
        string(REGEX REPLACE ";" "," src "${_src}")
        string(REGEX REPLACE ";" "," def "${_def}")
        string(REGEX REPLACE ";" "," opt "${_opt}")
        string(REGEX REPLACE ";" "," lnk "${_lnk}")
        string(REGEX REPLACE ";" "," dir "${_dir}")

        # TODO: be able to set other name than ext
        list(APPEND setup_extensions "Extension('${PYCMAKE_PACKAGE_NAME}.${ext}',
                                                sources=[${src}],
                                                include_dirs=[${inc}],
                                                define_macros=[${def}],
                                                library_dirs=[${dir}],
                                                libraries=[${lnk}],
                                                extra_compile_args=[${opt}])")

    endforeach()

    string(REGEX REPLACE ";" "," PYCMAKE_EXTENSIONS "${setup_extensions}")

    # When extensions are built, headers aren't typically included for source
    # dists, which are instead read from a manifest file. If a base is provided
    # we copy that, then append. If no template is provided, overwrite so it's
    # clean every time we append
    if (PP_MANIFEST)
        configure_file(${PP_MANIFEST} MANIFEST.in COPYONLY)
    else ()
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/MANIFEST.in)
    endif ()

    # Make a best-effort guess finding header files, trying all common
    # extensions
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/MANIFEST.in
                "recursive-include pycmake/include *.h *.hh *.H *.hpp *.hxx")

    set(setup.py setup.py)
    if (PP_OUTPUT)
        set(setup.py ${PP_OUTPUT})
    endif ()
    configure_file(${template} ${setup.py})
endfunction ()

function(add_python_test TESTNAME PYTHON_TEST_FILE)
    configure_file(${PYTHON_TEST_FILE} ${PYTHON_TEST_FILE} COPYONLY)
    get_filename_component(name ${PYTHON_TEST_FILE} NAME)
    get_filename_component(dir  ${PYTHON_TEST_FILE} DIRECTORY)

    add_test(NAME ${TESTNAME}
            COMMAND ${PYTHON_EXECUTABLE} -m unittest discover -vs ${dir} -p ${name}
            )
endfunction()

function(add_python_example pkg TESTNAME PYTHON_TEST_FILE)
    configure_file(${PYTHON_TEST_FILE} ${PYTHON_TEST_FILE} COPYONLY)

    add_test(NAME ${TESTNAME}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMAND ${PYTHON_EXECUTABLE} ${PYTHON_TEST_FILE} ${ARGN})

    get_target_property(buildpath ${pkg} PACKAGE_BUILD_PATH)
    pycmake_to_path_list(pythonpath "$ENV{PYTHONPATH}" ${buildpath})
    set_tests_properties(${TESTNAME} PROPERTIES ENVIRONMENT "PYTHONPATH=${pythonpath}")
endfunction()
