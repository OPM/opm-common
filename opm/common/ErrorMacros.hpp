/*
  Copyright 2013 Andreas Lauser
  Copyright 2009, 2010 SINTEF ICT, Applied Mathematics.
  Copyright 2009, 2010 Statoil ASA.

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
#ifndef OPM_ERRORMACROS_HPP
#define OPM_ERRORMACROS_HPP

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/gpuDecorators.hpp>

#include <string>
#include <exception>
#include <stdexcept>
#include <cassert>

// macros for reporting to stderr
#ifdef OPM_VERBOSE // Verbose mode
# include <iostream>
# define OPM_REPORT do { std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] " } while (false)
# define OPM_MESSAGE(x) do { OPM_REPORT; std::cerr << x << "\n"; } while (false)
# define OPM_MESSAGE_IF(cond, m) do {if(cond) OPM_MESSAGE(m);} while (false)
#else // non-verbose mode (default)
# define OPM_REPORT do {} while (false)
# define OPM_MESSAGE(x) do {} while (false)
# define OPM_MESSAGE_IF(cond, m) do {} while (false)
#endif

#if OPM_IS_INSIDE_HOST_FUNCTION
// Macro to throw an exception that counts as an error in PRT file.
// NOTE: For this macro to work, the
// exception class must exhibit a constructor with the signature
// (const std::string &message). Since this condition is not fulfilled
// for the std::exception, you should use this macro with some
// exception class derived from either std::logic_error or
// std::runtime_error.
//
// Usage: OPM_THROW(ExceptionClass, "Error message");
#define OPM_THROW(Exception, message)                          \
    do {                                                       \
        std::string oss_ = std::string{"["} + __FILE__ + ":" + \
                           std::to_string(__LINE__) + "] " +   \
                           message;                            \
        ::Opm::OpmLog::error(oss_);                            \
        throw Exception(oss_);                                 \
    } while (false)

// Macro to throw an exception that only counts as a problem in PRT file.
// NOTE: For this macro to work, the
// exception class must exhibit a constructor with the signature
// (const std::string &message). Since this condition is not fulfilled
// for the std::exception, you should use this macro with some
// exception class derived from either std::logic_error or
// std::runtime_error.
//
// Usage: OPM_THROW_PROBLEM(ExceptionClass, "Error message");
#define OPM_THROW_PROBLEM(Exception, message)                          \
    do {                                                       \
        std::string oss_ = std::string{"["} + __FILE__ + ":" + \
                           std::to_string(__LINE__) + "] " +   \
                           message;                            \
        ::Opm::OpmLog::problem(oss_);                            \
        throw Exception(oss_);                                 \
    } while (false)

// Same as OPM_THROW, except for not making an OpmLog::error() call.
//
// Usage: OPM_THROW_NOLOG(ExceptionClass, "Error message");
#define OPM_THROW_NOLOG(Exception, message)                    \
    do {                                                       \
        std::string oss_ = std::string{"["} + __FILE__ + ":" + \
                           std::to_string(__LINE__) + "] " +   \
                           message;                            \
        throw Exception(oss_);                                 \
    } while (false)

// throw an exception if a condition is true
#define OPM_ERROR_IF(condition, message) do {if(condition){ OPM_THROW(std::logic_error, message);}} while(false)

#else // On GPU
// On the GPU, we cannot throw exceptions, so we use assert(false) instead.
// This will allow us to keep the same code for both CPU and GPU when using
// the macros. The assert(false) will only cause the CUDA kernel to terminate, 
// but it will not throw an exception. However, later calls to cudaGetLastError
// or similar functions that check for errors will report the error, and if
// they are wrapped in the OPM_GPU_SAFE_CALL macro, that will throw an exception.
//
// Notice however, that once we assert(false) in a CUDA kernel, the CUDA context
// is broken for the rest of the process, see   
// https://forums.developer.nvidia.com/t/how-to-clear-cuda-errors/296393/5

/**
 * @brief assert(false) is only used on the GPU, as throwing exceptions is not supported.
 */
#define OPM_THROW(Exception, message) \
    assert(false)

/**
 * @brief assert(false) is only used on the GPU, as throwing exceptions is not supported.
 */
#define OPM_THROW_PROBLEM(Exception, message) \
   assert(false)

/**
 * @brief assert(false) is only used on the GPU, as throwing exceptions is not supported.
 */
#define OPM_THROW_NOLOG(Exception, message) \
   assert(false)

/**
 * @brief assert(condition) is only used on the GPU, as throwing exceptions is not supported.
 */
#define OPM_ERROR_IF(condition, message)                                    \
    do {if(condition){assert(false);}} while(false)
#endif // GPU
#endif // OPM_ERRORMACROS_HPP
