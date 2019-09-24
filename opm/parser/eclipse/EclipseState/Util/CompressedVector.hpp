/*
  Copyright 2019 Statoil ASA.

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

#ifndef OPM_COMPRESSED_VECTOR_HPP
#define OPM_COMPRESSED_VECTOR_HPP

#include <cstddef>
#include <vector>

namespace Opm {

template <typename T>
class CompressedVector {
public:

    CompressedVector(std::size_t size) :
        data_size(size)
    {
        this->extent_data.emplace_back(0, size, 0);
    }

    std::size_t size() const {
        return this->data_size;
    };


    std::vector<T> data() const {
        std::vector<T> d(this->data_size);
        for (const auto& ext : this->extent_data)
            std::fill_n(d.begin() + ext.start, ext.size, ext.value);
        return d;
    }


    void assign(const std::vector<T>& v) {
        printf("Starting assign ..... \n");
        if(v.size() != this->data_size)
            throw std::invalid_argument("Size mismatch");


        this->extent_data.clear();
        std::size_t start = 0;
        std::size_t sz = 0;
        T value = v[0];
        while (true) {
            if (start + sz == this->data_size) {
                this->extent_data.emplace_back(start, sz, value);
                break;
            }

            if (v[start + sz] != value) {
                this->extent_data.emplace_back(start, sz, value);
                start += sz;
                sz = 0;
                value = v[start];
            }
            sz += 1;
        }

        printf("Done %ld/%ld\n", this->extent_data.size(), v.size());
    }


private:

    struct extent {
        extent(std::size_t st, std::size_t sz, T v):
            start(st),
            size(sz),
            value(v)
        {};


        std::size_t start;
        std::size_t size;
        T value;
    };

    std::size_t data_size;
    std::vector<extent> extent_data;
};

}

#endif
