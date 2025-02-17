/*
  Copyright 2025 Equinor ASA

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

#ifndef OPM_COMMON_CRITICAL_ERROR_HPP
#define OPM_COMMON_CRITICAL_ERROR_HPP
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace Opm
/**
 * @file CriticalError.hpp
 * @brief Provides error handling mechanisms for critical errors in the OPM framework
 *
 * This file contains the CriticalError exception class and macros for handling
 * critical errors in a consistent way across the OPM codebase.
 *
 * The key components are:
 * - CriticalError class: A custom exception for critical errors
 * - OPM_CATCH_AND_RETHROW_AS_CRITICAL_ERROR macro: Catches and rethrows exceptions
 * - OPM_TRY_THROW_AS_CRITICAL_ERROR macro: Wraps code execution with error handling
 *
 * Both macros support an optional hint string that provides additional error context:
 * @code{.cpp}
 * try {
 *     riskyOperation();
 * }
 * OPM_CATCH_AND_RETHROW_AS_CRITICAL_ERROR("Check input file permissions");
 *
 * // Or using the try macro:
 * OPM_TRY_THROW_AS_CRITICAL_ERROR(riskyOperation(),
 *                                "Operation requires valid config");
 * @endcode
 *
 * When an error occurs, the macros will:
 * 1. Catch the exception
 * 2. Create a detailed error message including:
 *    - File and line location
 *    - Original error message
 *    - Optional hint string if provided
 * 3. Wrap in a CriticalError while preserving the original exception
 *
 * The hint string is especially useful for providing:
 * - Troubleshooting guidance
 * - Required preconditions
 * - Suggested fixes
 * - Context about the operation
 */
{
/**
 * @class CriticalError
 * @brief A custom exception class that extends std::exception to handle critical errors.
 *
 * This class provides a way to encapsulate critical error messages and optionally
 * store an inner exception for more detailed error information.
 */
class CriticalError : public std::exception
{
public:
    /**
     * @brief Constructs a CriticalError with a specified message and an optional inner exception.
     *
     * @param message A string_view containing the error message.
     * @param inner_exception An optional std::exception_ptr to an inner exception.
     */
    explicit CriticalError(const std::string_view& message,
                           std::exception_ptr inner_exception = nullptr)
        : m_message(message)
        , m_innerException(inner_exception)
    {
    }


    /**
     * @brief Returns the error message.
     *
     * @return A C-style string containing the error message.
     */
    const char* what() const noexcept override
    {
        return m_message.c_str();
    }

    /**
     * @brief Retrieves the inner exception.
     *
     * @return A std::exception_ptr to the inner exception, or nullptr if no inner exception is set.
     */
    std::exception_ptr getInnerException() const
    {
        return m_innerException;
    }

private:
    std::string m_message;
    std::exception_ptr m_innerException;
};

namespace detail
{
    inline std::string makeCriticalErrorMessageErrorHint(const std::string& text)
    {
        return std::string("\nError hint: ") + text;
    }

    inline std::string makeCriticalErrorMessageErrorHint()
    {
        return std::string("");
    }
} // namespace detail

/**
 * @brief Ends try-catch block and rethrows any caught exceptions as CriticalError.
 *
 * This macro is used to catch exceptions and rethrow them as CriticalError.
 * It also provides a way to add an optional hint string to the error message.
 *
 * @param ... An optional hint string to provide additional error context.
 */
#define OPM_CATCH_AND_RETHROW_AS_CRITICAL_ERROR(...)                                                                   \
    catch (const ::Opm::CriticalError&)                                                                                \
    {                                                                                                                  \
        throw;                                                                                                         \
    }                                                                                                                  \
    catch (const std::exception& exp)                                                                                  \
    {                                                                                                                  \
        const auto messageForCriticalError = std::string("Error rethrown as CriticalError at [") + __FILE__ + ":"      \
            + std::to_string(__LINE__) + "].\nOriginal error: " + exp.what()                                           \
            + ::Opm::detail::makeCriticalErrorMessageErrorHint(__VA_ARGS__);                                           \
        throw ::Opm::CriticalError(messageForCriticalError, std::current_exception());                                 \
    }                                                                                                                  \
    catch (...)                                                                                                        \
    {                                                                                                                  \
        const auto messageForCriticalError = std::string("Error rethrown as CriticalError at [") + __FILE__ + ":"      \
            + std::to_string(__LINE__) + "]. Unknown original error."                                                  \
            + ::Opm::detail::makeCriticalErrorMessageErrorHint(__VA_ARGS__);                                           \
        throw ::Opm::CriticalError(messageForCriticalError, std::current_exception());                                 \
    }

/**
 * @brief Tries to execute an expression and rethrows any caught exceptions as CriticalError.
 *
 * @param expr The expression to execute.
 * @param ... An optional hint string to provide additional error context.
 */
#define OPM_TRY_THROW_AS_CRITICAL_ERROR(expr, ...)                                                                     \
    try {                                                                                                              \
        expr;                                                                                                          \
    } catch (const ::Opm::CriticalError&) {                                                                            \
        throw;                                                                                                         \
    } catch (const std::exception& exp) {                                                                              \
        const auto messageForCriticalError = std::string("Error rethrown as CriticalError at [") + __FILE__ + ":"      \
            + std::to_string(__LINE__) + "].\nOriginal error: " + exp.what()                                           \
            + ::Opm::detail::makeCriticalErrorMessageErrorHint(__VA_ARGS__);                                           \
        throw ::Opm::CriticalError(messageForCriticalError, std::current_exception());                                 \
    } catch (...) {                                                                                                    \
        const auto messageForCriticalError = std::string("Error rethrown as CriticalError at [") + __FILE__ + ":"      \
            + std::to_string(__LINE__) + "]. Unknown original error."                                                  \
            + ::Opm::detail::makeCriticalErrorMessageErrorHint(__VA_ARGS__);                                           \
        throw ::Opm::CriticalError(messageForCriticalError, std::current_exception());                                 \
    }

} // namespace Opm
#endif // OPM_COMMON_CRITICAL_ERROR_HPP
