/*
  Copyright 2022 Equinor ASA.

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

#ifndef OPM_COPYABLE_PTR_HPP
#define OPM_COPYABLE_PTR_HPP
#include <opm/common/utility/gpuDecorators.hpp>
#include <memory>
namespace Opm {
namespace Utility {
#if 0
// Wraps std::unique_ptr and makes it copyable.
//
// WARNING: This template should not be used with polymorphic classes.
//  That would require a virtual clone() method to be implemented.
//  It will only ever copy the static class type of the pointed to class.
template <class T>
class CopyablePtr {
public:
    OPM_HOST_DEVICE CopyablePtr() : ptr_(nullptr) {}
    OPM_HOST_DEVICE  CopyablePtr(const CopyablePtr& other) {
        if (other) { // other does not contain a nullptr
            ptr_ = std::make_unique<T>(*other.get());
        }
        else {
            ptr_ = nullptr;
        }
    }
    // assignment operator
    OPM_HOST_DEVICE  CopyablePtr<T>& operator=(const CopyablePtr<T>& other) {
        if (other) {
            ptr_ = std::make_unique<T>(*other.get());
        }
        else {
            ptr_ = nullptr;
        }
        return *this;
    }
    // assign directly from a unique_ptr
    OPM_HOST_DEVICE  CopyablePtr<T>& operator=(std::unique_ptr<T>&& uptr) {
        ptr_ = std::move(uptr);
        return *this;
    }
    // member access operator
    OPM_HOST_DEVICE  T* operator->() const {return ptr_.get(); }
    // boolean context operator
    OPM_HOST_DEVICE  explicit operator bool() const noexcept {
        return ptr_ ? true : false;
    }
    // get a pointer to the stored value
    OPM_HOST_DEVICE  T* get() const {return ptr_.get();}
    OPM_HOST_DEVICE  T* release() const {return ptr_.release();}
private:
    std::unique_ptr<T> ptr_;
};
#else
template <class T>
class CopyablePtr {
public:
    OPM_HOST_DEVICE CopyablePtr() {}
    OPM_HOST_DEVICE CopyablePtr(const CopyablePtr& other) : m_value(other.m_value), m_has_value(other.m_has_value) {}
    
    // assignment operator
    OPM_HOST_DEVICE CopyablePtr<T>& operator=(const CopyablePtr<T>& other) {
        m_value = other.m_value;
        m_has_value = other.m_has_value;
        return *this;
    }

    // assignment operator
    OPM_HOST_DEVICE CopyablePtr<T>& operator=(const std::unique_ptr<T>& other) {
        if (other) {
            m_has_value = true;
            m_value = *other;
        } else {
            m_has_value = false;
        }
        return *this;
    }
    
    // member access operator
    OPM_HOST_DEVICE T* operator->() const {
        return get(); 
    }
    // boolean context operator
    OPM_HOST_DEVICE  explicit operator bool() const noexcept {
        return m_has_value;
    }
    // get a pointer to the stored value
    OPM_HOST_DEVICE T* get() const {
        if (m_has_value) {
            return const_cast<T*>(&m_value);
        } else {
            return nullptr;
        }
    }
    OPM_HOST_DEVICE T* release() const {
        return get();
    }
private:
    T m_value;
    bool m_has_value = false; 
};
#endif
} // namespace Utility
} // namespace Opm
#endif
