# parallel computing must be explicitly enabled
option (USE_MPI "Use Message Passing Interface for parallel computing" ON)
if (NOT USE_MPI)
  set (CMAKE_DISABLE_FIND_PACKAGE_MPI TRUE)
endif ()

# MPI normally reaches the OPM targets transitively through the DUNE modules.
# When DUNE is consumed from an install that does not re-export MPI on its
# targets (e.g. MS-MPI on Windows), it doesn't, and HAVE_MPI cannot be enabled.
# This opt-in makes mpi_checks() link MPI directly as a fallback. Defaulted ON
# only where that situation is expected, but overridable on any platform.
if (WIN32)
  set (_opm_link_mpi_directly_default ON)
else ()
  set (_opm_link_mpi_directly_default OFF)
endif ()
option (OPM_LINK_MPI_DIRECTLY
  "Link MPI directly onto the OPM target when the DUNE modules do not propagate it"
  ${_opm_link_mpi_directly_default})

function(mpi_checks)
  if(NOT USE_MPI)
    return()
  endif()
  cmake_parse_arguments(PARAM "" "TARGET" "" ${ARGN})
  if(NOT PARAM_TARGET)
    message(FATAL_ERROR "Function needs a TARGET parameter")
  endif()
  get_target_property(TARGET_LINKS ${PARAM_TARGET} INTERFACE_LINK_LIBRARIES)

  # Fallback for DUNE installs that don't propagate MPI (see OPM_LINK_MPI_DIRECTLY).
  # Only fires when MPI is genuinely absent from the target, so a normal build
  # where DUNE already provides MPI is untouched.
  if(OPM_LINK_MPI_DIRECTLY AND
     NOT (TARGET_LINKS MATCHES "libmpi${CMAKE_SHARED_LIBRARY_SUFFIX}" OR
          TARGET_LINKS MATCHES "MPI::MPI_C"))
    find_package(MPI COMPONENTS C)
    if(MPI_C_FOUND)
      target_link_libraries(${PARAM_TARGET} PUBLIC MPI::MPI_C)
      get_target_property(TARGET_LINKS ${PARAM_TARGET} INTERFACE_LINK_LIBRARIES)
    else()
      message(WARNING "USE_MPI and OPM_LINK_MPI_DIRECTLY are ON but MPI was not found; "
                      "building ${PARAM_TARGET} without MPI support.")
    endif()
  endif()

  if(TARGET_LINKS MATCHES "libmpi${CMAKE_SHARED_LIBRARY_SUFFIX}" OR
     TARGET_LINKS MATCHES MPI::MPI_C)
    # check for MPI version 2
    include(CMakePushCheckState)
    include(CheckFunctionExists)
    cmake_push_check_state()
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES};${MPI_C_LIBRARIES})
    set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES};${MPI_C_INCLUDES})
    check_function_exists(MPI_Finalized MPI_2)
    cmake_pop_check_state()
    target_compile_definitions(${PARAM_TARGET}
      PUBLIC
        HAVE_MPI=1
        MPI_2=${MPI_2}
    )
  endif()
endfunction()
