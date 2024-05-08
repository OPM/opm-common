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

// HAVE_CUDA and USE_HIP are found in config.h
#ifndef OPM_GPUDECORATORS_HPP
  #define OPM_GPUDECORATORS_HPP
  #include <config.h>

  #if HAVE_CUDA // if we will compile with GPU support

    //handle inclusion of correct gpu runtime headerfiles
    #if USE_HIP // if we compile for AMD architectures
      #include <hip/hip_runtime.h>
    #else // if we compile for Nvidia architectures
      #include <cuda_runtime.h>
    #endif // END USE_HIP

    // define host and device function attributes
    #define OPM_HOST_DEVICE __device__ __host__
    #define OPM_DEVICE __device__
    #define OPM_HOST __host__
    // define a variable keeping track of whether or not we are compiling for GPUs or not
    #define OPM_IS_USING_GPU 1

    // add a variable that can keep track of whether or not we are compiling with hipcc/nvcc (gpu compilers)
    #if defined(__NVCC__) || defined(__HIPCC__)
      #define OPM_IS_COMIPING_WITH_GPU_COMPILER 1
    #endif // END IF GPUCC

    // add a variable keeping track of whether or not it is placed inside a function marked __device__
    #if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
      #define OPM_IS_INSIDE_DEVICE_FUNCTION (defined(__CUDA_ARCH__) || (defined(__HIP_DEVICE_COMPILE__) && __HIP_DEVICE_COMPILE__ > 0))
    #endif

  #else // if we are not using CUDA/HIP, let the macros be empty
    #define OPM_HOST_DEVICE
    #define OPM_DEVICE
    #define OPM_HOST
    #define OPM_IS_COMIPING_WITH_GPU_COMPILER
    #define OPM_IS_USING_GPU
    #define OPM_IS_INSIDE_DEVICE_FUNCTION
  #endif // END ELSE

#endif // END HEADER GUARD
