# - Find HYPRE library
#
# This module sets the following variables:
#  HYPRE_FOUND        - True if HYPRE was found
#  HYPRE_INCLUDE_DIRS - Include directories for HYPRE
#  HYPRE::HYPRE       - Imported target for HYPRE
#  HYPRE_USING_CUDA   - True if HYPRE was built with CUDA support
#  HYPRE_USING_HIP    - True if HYPRE was built with HIP support

# Look for the header files
find_path(HYPRE_INCLUDE_DIRS
  NAMES HYPRE.h
  HINTS
  ${HYPRE_DIR}
  ${HYPRE_DIR}/include
  ${HYPRE_DIR}/hypre/include
  $ENV{HYPRE_DIR}
  /usr/include
  /usr/local/include
  /usr/include/hypre
  /opt
  /opt/local
  PATH_SUFFIXES hypre
  DOC "Directory containing HYPRE header files")

# Find the HYPRE library
find_library(HYPRE_LIBRARY
  NAMES HYPRE hypre
  HINTS
    ${HYPRE_DIR}
    ${HYPRE_DIR}/lib
    ${HYPRE_DIR}/hypre/lib
    $ENV{HYPRE_DIR}
    /usr/lib
    /usr/lib/x86_64-linux-gnu
    /usr/local/lib
  PATH_SUFFIXES lib lib64
  DOC "HYPRE library")

find_package(MPI QUIET REQUIRED)
find_package(LAPACK QUIET REQUIRED)
find_package(BLAS QUIET REQUIRED)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HYPRE
                                  REQUIRED_VARS
                                  HYPRE_LIBRARY
                                  HYPRE_INCLUDE_DIRS)

if(HYPRE_FOUND AND NOT TARGET HYPRE::HYPRE)
  file(STRINGS ${HYPRE_INCLUDE_DIRS}/HYPRE_config.h HYPRE_CONFIG)
  string(FIND "${HYPRE_CONFIG}" "#define HYPRE_USING_CUDA" CUDA_POS)
  if(NOT CUDA_POS EQUAL -1)
    set(HYPRE_USING_CUDA ON)
  endif()
  string(FIND "${HYPRE_CONFIG}" "#define HYPRE_USING_HIP" HIP_POS)
  if(NOT HIP_POS EQUAL -1)
    set(HYPRE_USING_HIP ON)
  endif()

  add_library(HYPRE::HYPRE UNKNOWN IMPORTED GLOBAL)
  set_target_properties(HYPRE::HYPRE PROPERTIES
    IMPORTED_LOCATION "${HYPRE_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${HYPRE_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${MPI_C_LIBRARIES};${LAPACK_LIBRARIES};${BLAS_LIBRARIES}")

  if(HYPRE_USING_CUDA)
    find_package(CUDAToolkit REQUIRED)
    set_property(TARGET HYPRE::HYPRE APPEND PROPERTY
      INTERFACE_COMPILE_DEFINITIONS HYPRE_USING_CUDA)
    set_property(TARGET HYPRE::HYPRE APPEND PROPERTY
      INTERFACE_LINK_LIBRARIES
        CUDA::cudart
        CUDA::cusparse
        CUDA::curand
        CUDA::cublas)
    message(STATUS "HYPRE was built with CUDA support")
  endif()

  if(HYPRE_USING_HIP)
    find_package(rocsparse REQUIRED)
    find_package(rocrand REQUIRED)
    find_package(rocblas REQUIRED)
    set_property(TARGET HYPRE::HYPRE APPEND PROPERTY
      INTERFACE_COMPILE_DEFINITIONS HYPRE_USING_HIP)
    set_property(TARGET HYPRE::HYPRE APPEND PROPERTY
      INTERFACE_LINK_LIBRARIES
        roc::rocsparse
        roc::rocrand
        roc::rocblas)
    message(STATUS "HYPRE was built with HIP support")
  endif()
endif()
