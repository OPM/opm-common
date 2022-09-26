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
namespace Opm {
namespace Utility {
// Wraps std::unique_ptr and makes it copyable.
//
// WARNING: This template should not be used with polymorphic classes.
//  That would require a virtual clone() method to be implemented.
//  It will only ever copy the static class type of the pointed to class.
template <class T>
class CopyablePtr {
public:
    CopyablePtr() : ptr_(nullptr) {}
    CopyablePtr(const CopyablePtr& other) {
        if (other) { // other does not contain a nullptr
            ptr_ = std::make_unique<T>(*other.get());
        }
        else {
            ptr_ = nullptr;
        }
    }
    // assignment operator
    CopyablePtr<T>& operator=(const CopyablePtr<T>& other) {
        if (other) {
            ptr_ = std::make_unique<T>(*other.get());
        }
        else {
            ptr_ = nullptr;
        }
        return *this;
    }
    // assign directly from a unique_ptr
    CopyablePtr<T>& operator=(std::unique_ptr<T>&& uptr) {
        ptr_ = std::move(uptr);
        return *this;
    }
    // member access operator
    T* operator->() const {return ptr_.get(); }
    // boolean context operator
    explicit operator bool() const noexcept {
        return ptr_ ? true : false;
    }
    // get a pointer to the stored value
    T* get() const {return ptr_.get();}
    T* release() const {return ptr_.release();}
private:
    std::unique_ptr<T> ptr_;
};

} // namespace Utility
} // namespace Opm
#endif