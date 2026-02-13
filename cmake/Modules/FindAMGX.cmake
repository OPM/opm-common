# Find AMGX library
#
# Sets the following variables:
#   AMGX_FOUND        - True if AMGX was found
#   AMGX_INCLUDE_DIRS - Include directories for AMGX
#   AMGX_LIBRARIES    - Libraries for AMGX

# AMGX requires CUDA
find_package(CUDAToolkit REQUIRED)

# Try to find AMGX in standard paths or user-specified AMGX_DIR
find_path(AMGX_INCLUDE_DIR
    NAMES amgx_c.h
    PATHS
        ${AMGX_DIR}
        ${AMGX_DIR}/include
        ${AMGX_DIR}/../include
        ENV AMGX_DIR
    PATH_SUFFIXES include
    DOC "AMGX include directory"
)

# Find both static and shared libraries
find_library(AMGX_LIBRARY
    NAMES amgx libamgx.a libamgxsh.so
    PATHS
        ${AMGX_DIR}
        ${AMGX_DIR}/build
        ENV AMGX_DIR
    PATH_SUFFIXES lib lib64
    DOC "AMGX library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AMGX
    REQUIRED_VARS
    AMGX_INCLUDE_DIR
    AMGX_LIBRARY
    VERSION_VAR AMGX_VERSION
)

if(AMGX_FOUND)
    set(AMGX_INCLUDE_DIRS ${AMGX_INCLUDE_DIR})
    set(AMGX_LIBRARIES ${AMGX_LIBRARY})

    if(NOT TARGET AMGX::AMGX)
        add_library(AMGX::AMGX UNKNOWN IMPORTED)
        set_target_properties(AMGX::AMGX PROPERTIES
            IMPORTED_LOCATION "${AMGX_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${AMGX_INCLUDE_DIRS}"
        )

        # Since AMGX is basically unversioned we need to use cuda version to decide
        if(CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 12.9)
          set(EXTRA_LIBS CUDA::nvToolsExt)
        endif()

        # Link CUDA libraries that AMGX depends on
        target_link_libraries(AMGX::AMGX INTERFACE
            CUDA::cusolver
            CUDA::cusparse
            CUDA::cublas
            CUDA::cudart
            ${EXTRA_LIBS}
        )
    endif()
endif()
