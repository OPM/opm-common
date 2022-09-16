/*
  Copyright 2019 Equinor AS.

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
#ifndef SIMPLE_PACKER_HPP
#define SIMPLE_PACKER_HPP

#include <opm/common/utility/TimeService.hpp>

#include <bitset>
#include <cstddef>
#include <cstring>
#include <string>

namespace Opm {
namespace TestUtil {
namespace detail {

//! \brief Abstract struct for packing which is (partially) specialized for specific types.
template <bool pod, class T>
struct Packing
{
    static std::size_t packSize(const T&);
    static void pack(const T&, std::vector<char>&, int&);
    static void unpack(T&, std::vector<char>&, int&);
};

//! \brief Packaging for pod data.
template<class T>
struct Packing<true,T>
{
    //! \brief Calculates the pack size for a POD.
    //! \param data The data to pack
    static std::size_t packSize(const T& data)
    {
        return packSize(&data, 1);
    }

    //! \brief Calculates the pack size for an array of POD.
    //! \param data The array to pack
    //! \param n Length of array
    static std::size_t packSize(const T*, std::size_t n)
    {
        return n*sizeof(T);
    }

    //! \brief Pack a POD.
    //! \param data The variable to pack
    //! \param buffer Buffer to pack into
    //! \param position Position in buffer to use
    static void pack(const T& data,
                     std::vector<char>& buffer,
                     int& position)
    {
        pack(&data, 1, buffer, position);
    }

    //! \brief Pack an array of POD.
    //! \param data The array to pack
    //! \param n Length of array
    //! \param buffer Buffer to pack into
    //! \param position Position in buffer to use
    static void pack(const T* data,
                     std::size_t n,
                     std::vector<char>& buffer,
                     int& position)
    {
        std::memcpy(&buffer[position], data, n*sizeof(T));
        position += n*sizeof(T);
    }

    //! \brief Unpack a POD.
    //! \param data The variable to unpack
    //! \param buffer Buffer to unpack from
    //! \param position Position in buffer to use
    static void unpack(T& data,
                       std::vector<char>& buffer,
                       int& position)
    {
        unpack(&data, 1, buffer, position);
    }

    //! \brief Unpack an array of POD.
    //! \param data The array to unpack
    //! \param n Length of array
    //! \param buffer Buffer to unpack from
    //! \param position Position in buffer to use
    static void unpack(T* data,
                       std::size_t n,
                       std::vector<char>& buffer,
                       int& position)
    {
        std::memcpy(data, &buffer[position], n*sizeof(T));
        position += n*sizeof(T);
    }
};

//! \brief Default handling for unsupported types.
template<class T>
struct Packing<false,T>
{
    static std::size_t packSize(const T&)
    {
        static_assert(!std::is_same_v<T,T>, "Packing not supported for type");
        return 0;
    }

    static void pack(const T&, std::vector<char>&, int&)
    {
        static_assert(!std::is_same_v<T,T>, "Packing not supported for type");
    }

    static void unpack(T&, std::vector<char>&, int&)
    {
        static_assert(!std::is_same_v<T,T>, "Packing not supported for type");
    }
};

//! \brief Specialization for std::bitset
template <std::size_t Size>
struct Packing<false,std::bitset<Size>>
{
    static std::size_t packSize(const std::bitset<Size>& data)
    {
        return Packing<true,unsigned long long>::packSize(data.to_ullong());
    }

    static void pack(const std::bitset<Size>& data,
                     std::vector<char>& buffer, int& position)
    {
        Packing<true,unsigned long long>::pack(data.to_ullong(), buffer, position);
    }

    static void unpack(std::bitset<Size>& data,
                       std::vector<char>& buffer, int& position)
    {
        unsigned long long d;
        Packing<true,unsigned long long>::unpack(d, buffer, position);
        data = std::bitset<Size>(d);
    }
};

template<>
struct Packing<false,std::string>
{
    static std::size_t packSize(const std::string& data)
    {
        return sizeof(std::size_t) + data.size();
    }

    static void pack(const std::string& data,
                     std::vector<char>& buffer, int& position)
    {
        Packing<true,std::size_t>::pack(data.size(), buffer, position);
        Packing<true,char>::pack(data.data(), data.size(), buffer, position);
    }

    static void unpack(std::string& data, std::vector<char>& buffer, int& position)
    {
        std::size_t length = 0;
        Packing<true,std::size_t>::unpack(length, buffer, position);
        std::vector<char> cStr(length+1, '\0');
        Packing<true,char>::unpack(cStr.data(), length, buffer, position);
        data.clear();
        data.append(cStr.data(), length);
    }
};

template<>
struct Packing<false,time_point>
{
    static std::size_t packSize(const time_point&)
    {
        return Packing<true,std::time_t>::packSize(std::time_t());
    }

    static void pack(const time_point& data,
                     std::vector<char>& buffer, int& position)
    {
        Packing<true,std::time_t>::pack(TimeService::to_time_t(data),
                                        buffer, position);
    }

    static void unpack(time_point& data, std::vector<char>& buffer, int& position)
    {
        std::time_t res;
        Packing<true,std::time_t>::unpack(res, buffer, position);
        data = TimeService::from_time_t(res);
    }
};

}

//! \brief Struct handling packing of serialization for test purposes.
struct Packer {
    //! \brief Calculates the pack size for a variable.
    //! \tparam T The type of the data to be packed
    //! \param data The data to pack
    template<class T>
    std::size_t packSize(const T& data) const
    {
        return detail::Packing<std::is_pod_v<T>,T>::packSize(data);
    }

    //! \brief Calculates the pack size for an array.
    //! \tparam T The type of the data to be packed
    //! \param data The array to pack
    //! \param n Length of array
    template<class T>
    std::size_t packSize(const T* data, std::size_t n) const
    {
        static_assert(std::is_pod_v<T>, "Array packing not supported for non-pod data");
        return detail::Packing<true,T>::packSize(data, n);
    }

    //! \brief Pack a variable.
    //! \tparam T The type of the data to be packed
    //! \param data The variable to pack
    //! \param buffer Buffer to pack into
    //! \param position Position in buffer to use
    template<class T>
    void pack(const T& data,
              std::vector<char>& buffer,
              int& position) const
    {
        detail::Packing<std::is_pod_v<T>,T>::pack(data, buffer, position);
    }

    //! \brief Pack an array.
    //! \tparam T The type of the data to be packed
    //! \param data The array to pack
    //! \param n Length of array
    //! \param buffer Buffer to pack into
    //! \param position Position in buffer to use
    template<class T>
    void pack(const T* data,
              std::size_t n,
              std::vector<char>& buffer,
              int& position) const
    {
        static_assert(std::is_pod_v<T>, "Array packing not supported for non-pod data");
        detail::Packing<true,T>::pack(data, n, buffer, position);
    }

    //! \brief Unpack a variable.
    //! \tparam T The type of the data to be unpacked
    //! \param data The variable to unpack
    //! \param buffer Buffer to unpack from
    //! \param position Position in buffer to use
    template<class T>
    void unpack(T& data,
                std::vector<char>& buffer,
                int& position) const
    {
        detail::Packing<std::is_pod_v<T>,T>::unpack(data, buffer, position);
    }

    //! \brief Unpack an array.
    //! \tparam T The type of the data to be unpacked
    //! \param data The array to unpack
    //! \param n Length of array
    //! \param buffer Buffer to unpack from
    //! \param position Position in buffer to use
    template<class T>
    void unpack(T* data,
                std::size_t n,
                std::vector<char>& buffer,
                int& position) const
    {
        static_assert(std::is_pod_v<T>, "Array packing not supported for non-pod data");
        detail::Packing<true,T>::unpack(data, n, buffer, position);
    }
};

} // end namespace TestUtil
} // end namespace Opm

#endif // SIMPLE_PACKER_HPP
