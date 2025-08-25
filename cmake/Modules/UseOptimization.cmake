# - Turn on optimizations based on build type

include(TestCXXAcceptsFlag)
include (AddOptions)
include (UseCompVer)
is_compiler_gcc_compatible ()

# mapping from profile name (in CMAKE_BUILD_TYPE) to variable part
set (_prof_DEBUG "Debug")
set (_prof_RELEASE "Release;RelWithDebInfo;MinSizeRel")

# if we are building a debug target, then disable all optimizations
# otherwise, turn them on. indicate to the code what we have done
# so it can turn on assertions etc.

if (CXX_COMPAT_GCC)
  # extra flags passed for optimization
  set (_opt_flags "")

  # native instruction set tuning
  option (WITH_NATIVE "Use native instruction set" ON)
  if (WITH_NATIVE)
	check_cxx_accepts_flag ("-mtune=native" HAVE_MTUNE)
	if (HAVE_MTUNE)
	  list (APPEND _opt_flags "-mtune=native")
	endif (HAVE_MTUNE)
  endif (WITH_NATIVE)

  # default optimization flags, if not set by user
  set_default_option (CXX _opt_dbg "-O0" "(^|\ )-O")
  set_default_option (CXX _opt_rel "-O3" "(^|\ )-O")

  # use these options for debug builds - no optimizations
  add_options (ALL_LANGUAGES "${_prof_DEBUG}" ${_opt_dbg} "-DDEBUG")

  # use these options for release builds - full optimization
  add_options (ALL_LANGUAGES "${_prof_RELEASE}" ${_opt_rel} ${_opt_flags})
  option(WITH_NDEBUG "Disable asserts in release mode" OFF)
  if(WITH_NDEBUG)
    foreach(type ${_prof_RELEASE})
      string(TOUPPER ${type} type)
      string(REPLACE -UNDEBUG "" CMAKE_CXX_FLAGS_${type} "${CMAKE_CXX_FLAGS_${type}}")
      string(REPLACE -UNDEBUG "" CMAKE_C_FLAGS_${type} "${CMAKE_C_FLAGS_${type}}")
      string(REPLACE -UNDEBUG "" CMAKE_Fortran_FLAGS_${type} "${CMAKE_Fortran_FLAGS_${type}}")
    endforeach()
  else()
    add_options (ALL_LANGUAGES "${_prof_RELEASE}" -UNDEBUG)
  endif()

else ()
  # default information from system
  foreach (lang IN ITEMS C CXX Fortran)
	if (lang STREQUAL "Fortran")
	  set (_lang F)
	else (lang STREQUAL "Fortran")
	  set (_lang ${lang})
	endif (lang STREQUAL "Fortran")
	foreach (profile IN ITEMS DEBUG RELEASE)
	  if (NOT CMAKE_${lang}_FLAGS_${profile})
		add_options (${lang} "${_prof_${profile}}"
		  "$ENV{${_lang}FLAGS} ${CMAKE_${lang}_FLAGS_${profile}_INIT}")
	  endif (NOT CMAKE_${lang}_FLAGS_${profile})
	endforeach (profile)
  endforeach (lang)
endif ()
