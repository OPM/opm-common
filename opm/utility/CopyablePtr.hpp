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

#include <memory>
#include <type_traits>
#include <utility>
#include <opm/common/utility/gpuDecorators.hpp>

namespace Opm {
namespace Utility {

// Wraps a raw pointer and makes it copyable, with GPU support.
//
// On the host: owns the pointed-to object (deep copies on copy-construct/assign,
//   destroys on destruction) — same semantics as the original unique_ptr wrapper.
// On the device (CUDA/HIP kernel): behaves as a non-owning view; copy/assign
//   simply copy the raw pointer and the destructor is a no-op.
//
// WARNING: This template should not be used with polymorphic classes.
//   That would require a virtual clone() method. It will only ever copy
//   the static class type of the pointed-to object.
template <class T, bool on_gpu = OPM_IS_INSIDE_DEVICE_FUNCTION>
class CopyablePtr {
public:
    template <class U, bool B>
    friend class CopyablePtr;

    using ptr_type = std::conditional_t<on_gpu, T*, std::unique_ptr<T>>;

    OPM_HOST_DEVICE CopyablePtr() : ptr_(nullptr) {}

    OPM_HOST_DEVICE CopyablePtr(const CopyablePtr& other) {
        if constexpr (on_gpu) {
            ptr_ = other.ptr_;
        } else {
            ptr_ = other ? std::make_unique<T>(*other.get()) : nullptr;
        }
    }

    OPM_HOST_DEVICE CopyablePtr(CopyablePtr&& other) noexcept {
        if constexpr (on_gpu) {
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        } else {
            ptr_ = std::move(other.ptr_);
        }
    }

    // copy assignment
    OPM_HOST_DEVICE CopyablePtr& operator=(const CopyablePtr& other) {
        if (this != &other) {
            if constexpr (on_gpu) {
                ptr_ = other.ptr_;
            } else {
                ptr_ = other ? std::make_unique<T>(*other.get()) : nullptr;
            }
        }
        return *this;
    }

    // move assignment
    OPM_HOST_DEVICE CopyablePtr& operator=(CopyablePtr&& other) noexcept {
        if (this != &other) {
            if constexpr (on_gpu) {
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
            } else {
                ptr_ = std::move(other.ptr_);
            }
        }
        return *this;
    }

    // Assign directly from a unique_ptr.
    //
    // This overload is host-only when on_gpu == true: the GPU-view
    // configuration has a no-op destructor, so taking ownership of a
    // \c unique_ptr<T> from the host would leak the object. We therefore
    // delete this overload in the host pass for GPU-view instantiations
    // (so user code can only construct GPU views from non-owning raw
    // pointers via \c to_gpu_view()).
    //
    // In a device-compiler pass (CUDA/HIP) we must still allow the
    // overload to compile, because host-only code that is transitively
    // instantiated for the device pass (e.g. \c FlowProblem::updateRelperms
    // with directional mobilities) names it. Such code is never executed
    // on the device, so the historical \c release()-and-leak fallback is
    // harmless there.
#if OPM_IS_INSIDE_DEVICE_FUNCTION
    CopyablePtr& operator=(std::unique_ptr<T>&& uptr) {
        if constexpr (on_gpu) {
            ptr_ = uptr.release();
        } else {
            ptr_ = std::move(uptr);
        }
        return *this;
    }
#else
    CopyablePtr& operator=(std::unique_ptr<T>&& uptr)
        requires (!on_gpu)
    {
        ptr_ = std::move(uptr);
        return *this;
    }
#endif

    OPM_HOST_DEVICE ~CopyablePtr() = default;

    // member access operator
    OPM_HOST_DEVICE T* operator->() const { return deref_ptr(); }

    // boolean context operator
    OPM_HOST_DEVICE explicit operator bool() const noexcept { return deref_ptr() != nullptr; }

    // get a raw pointer to the stored value
    OPM_HOST_DEVICE T* get() const { return deref_ptr(); }

    // release ownership
    T* release() {
        if constexpr (on_gpu) {
            T* tmp = ptr_;
            ptr_ = nullptr;
            return tmp;
        } else {
            return ptr_.release();
        }
    }

    // Build a non-owning GPU view of the stored pointer.
    OPM_HOST_DEVICE CopyablePtr<T, true> to_gpu_view() const {
        return CopyablePtr<T, true>(this->get());
    }

private:
    explicit OPM_HOST_DEVICE CopyablePtr(T* ptr) : ptr_(ptr) {}

    OPM_HOST_DEVICE T* deref_ptr() const {
        if constexpr (on_gpu) {
            return ptr_;
        } else {
            return ptr_.get();
        }
    }

    ptr_type ptr_;
};

} // namespace Utility
} // namespace Opm
#endif
