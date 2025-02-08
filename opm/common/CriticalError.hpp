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
     * @brief Constructs a CriticalError with an optional inner exception.
     *
     * @param inner_exception An optional std::exception_ptr to an inner exception.
     */
    CriticalError(std::exception_ptr inner_exception = nullptr)
        : m_message("Unknown error message.")
        , m_innerException(inner_exception)
    {
    }

    /**
     * @brief Constructs a CriticalError with a specified message and an optional inner exception.
     *
     * @param message A string_view containing the error message.
     * @param inner_exception An optional std::exception_ptr to an inner exception.
     */

    CriticalError(const std::string_view& message, std::exception_ptr inner_exception = nullptr)
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

/**
 * @brief Begins try-catch block and rethrows any caught exceptions as CriticalError.
 */
#define OPM_BEGIN_TRY_CATCH_RETHROW_AS_CRITICAL_ERROR() try {

/**
 * @brief Ends try-catch block and rethrows any caught exceptions as CriticalError.
 */
#define OPM_END_TRY_CATCH_RETHROW_AS_CRITICAL_ERROR()                                                                  \
    }                                                                                                                  \
    catch (const std::exception& exp)                                                                                  \
    {                                                                                                                  \
        throw Opm::CriticalError(exp.what(), std::current_exception());                                                \
    }                                                                                                                  \
    catch (...)                                                                                                        \
    {                                                                                                                  \
        throw Opm::CriticalError(std::current_exception());                                                            \
    }

/**
 * @brief Tries to execute an expression and rethrows any caught exceptions as CriticalError.
 *
 * @param expr The expression to execute.
 */
#define OPM_TRY_THROW_AS_CRITICAL_ERROR(expr)                                                                          \
    try {                                                                                                              \
        expr;                                                                                                          \
    } catch (const std::exception& exp) {                                                                              \
        throw Opm::CriticalError(exp.what(), std::current_exception());                                                \
    } catch (...) {                                                                                                    \
        throw Opm::CriticalError(std::current_exception());                                                            \
    }

} // namespace Opm
#endif // OPM_COMMON_CRITICAL_ERROR_HPP