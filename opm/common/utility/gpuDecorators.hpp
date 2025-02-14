/*
  Copyright 2024 SINTEF Digital

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
    The point of this file is defining macros like OPM_HOST_DEVICE to be empty if
    we do not compile for GPU architectures, otherwise we will
    set it to "__device__ __host__" to decorate functions that can
    be called from a hip/cuda kernel.

    The file also provides some definitions that will probably become useful later,
    such as OPM_IS_INSIDE_DEVICE_FUNCTION for specializing code that can be called
    from both GPUs and CPUs.
*/

/// Decorators for GPU functions
///
/// @note This file requires that config.h has already been included
#ifndef OPM_GPUDECORATORS_HPP
  #define OPM_GPUDECORATORS_HPP

  // On CUDA we get some warnings that will yield compilation regardless, so we can ignore them
  #ifdef __CUDACC__
  #pragma nv_diag_suppress 20011,20014
  #endif

  //TODO Should probably include config.h if config.h becomes installable

  // true if using nvcc/hipcc gpu compiler
  #if defined(__NVCC__) || defined(__HIPCC__)
  #define OPM_IS_COMPILING_WITH_GPU_COMPILER 1
  #else
  #define OPM_IS_COMPILING_WITH_GPU_COMPILER 0
  #endif

  // true inside device version of functions marked __device__
  #if defined(__CUDA_ARCH__) || (defined(__HIP_DEVICE_COMPILE__) && __HIP_DEVICE_COMPILE__ > 0)
  #define OPM_IS_INSIDE_DEVICE_FUNCTION 1
  #define OPM_IS_INSIDE_HOST_FUNCTION 0
  #else
  #define OPM_IS_INSIDE_DEVICE_FUNCTION 0
  #define OPM_IS_INSIDE_HOST_FUNCTION 1
  #endif

  #if HAVE_CUDA // if we will compile with GPU support

    //handle inclusion of correct gpu runtime headerfiles
    #if USE_HIP // use HIP if we compile for AMD architectures
      #include <hip/hip_runtime.h>
    #else // otherwise include cuda
      #include <cuda_runtime.h>
    #endif // END USE_HIP

    #define OPM_HOST_DEVICE __device__ __host__
    #define OPM_DEVICE __device__
    #define OPM_HOST __host__
    #define OPM_IS_USING_GPU 1
  #else
    #define OPM_HOST_DEVICE
    #define OPM_DEVICE
    #define OPM_HOST
    #define OPM_IS_USING_GPU 0
  #endif // END ELSE

#endif // END HEADER GUARD
