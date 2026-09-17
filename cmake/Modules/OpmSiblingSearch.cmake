option (SIBLING_SEARCH "Search sibling directories before system paths" ON)
mark_as_advanced (SIBLING_SEARCH)

# NOTE: opm-grid and opm-simulators duplicate this logic inline in their
#    CMakeLists.txt because they need to locate opm-common *before*
#    find_package(opm-common), so this module is not yet available.
#    If you change the search logic here, update the inline copies too.
# DESCRIPTION:
# Try to infer <module>_DIR (i.e., the build directory of a sibling module)
# by inspecting the current project's source/build layout. This is useful
# when working with multiple sibling repositories (e.g., opm-grid,
# opm-simulators) without explicitly setting *_DIR variables.
#
# The macro attempts three strategies, in order:
#
# 1) Matching relative build paths inside sibling repositories
#    Assumes each module is built in the same relative location within its
#    own source tree.
#    Example:
#      /work/opm-grid/builds/debug
#      /work/opm-simulators/builds/debug
#    → reuse the relative path ("builds/debug") for the sibling repo.
#
# 2) Sibling build directories with similar naming patterns
#    Assumes build directories are siblings in the same parent directory,
#    and their names differ only by module name.
#    Example:
#      /work/builds/build-opm-grid-debug
#      /work/builds/build-opm-simulators-debug
#    → replace PROJECT_NAME with <module> in the leaf directory name.
#
# 3) Common build root containing per-module subdirectories
#    Assumes all modules are built under a shared directory, with each
#    module having its own subdirectory named after the module.
#    Example:
#      /work/build/opm-grid/
#      /work/build/opm-simulators/
#    → use <parent>/<module> if it looks like a CMake build dir
#      (contains CMakeCache.txt).
#
# The first matching strategy that finds an existing directory is used.
# If none match, <module>_DIR is left unset.
macro(create_module_dir_var module)
  if(SIBLING_SEARCH AND NOT ${module}_DIR)
    set(_clone_dir "${module}")
    # Strategy 1: Compute relative path from source to build dir, apply to sibling.
    # Handles both flat (repo/build) and nested (repo/builds/debug) layouts.
    get_filename_component(_source_parent "${PROJECT_SOURCE_DIR}" DIRECTORY)
    file(RELATIVE_PATH _build_rel "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
    set(_sibling_build "${_source_parent}/${_clone_dir}/${_build_rel}")
    if(NOT _build_rel MATCHES "^\\.\\." AND IS_DIRECTORY "${_sibling_build}")
      set(${module}_DIR "${_sibling_build}")
    else()
      # Fallback strategies for out-of-source or non-standard layouts
      get_filename_component(_leaf_dir_name "${PROJECT_BINARY_DIR}" NAME)
      get_filename_component(_parent_full_dir "${PROJECT_BINARY_DIR}" DIRECTORY)
      # Strategy 2: Build dirs named <prefix><module><postfix>
      string(REPLACE "${PROJECT_NAME}" "${_clone_dir}" _module_leaf "${_leaf_dir_name}")
      if(NOT _leaf_dir_name STREQUAL _module_leaf
          AND IS_DIRECTORY "${_parent_full_dir}/${_module_leaf}")
        set(${module}_DIR "${_parent_full_dir}/${_module_leaf}")
      # Strategy 3: All modules in a common build dir
      elseif(IS_DIRECTORY "${_parent_full_dir}/${_clone_dir}" AND
          EXISTS "${_parent_full_dir}/${_clone_dir}/CMakeCache.txt")
        set(${module}_DIR "${_parent_full_dir}/${_clone_dir}")
      endif()
    endif()
  endif()
  if(${module}_DIR AND NOT IS_DIRECTORY ${${module}_DIR})
    message(WARNING "Value ${${module}_DIR} passed to variable"
      " ${module}_DIR is not a directory")
  endif()
endmacro()
