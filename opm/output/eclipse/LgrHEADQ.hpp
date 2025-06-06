/*
  Copyright 2021-2024 Equinor ASA.
  Copyright 2016, 2017, 2018 Statoil ASA.

  This file is part of the Open Porous Media Project (OPM).

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

#ifndef OPM_LGRHEADQ_HEADER_INCLUDED
#define OPM_LGRHEADQ_HEADER_INCLUDED

#include <ctime>
#include <vector>

namespace Opm {

} // namespace Opm

namespace Opm { namespace RestartIO {

    class LgrHEADQ
    {
    public:
        LgrHEADQ();
        ~LgrHEADQ() = default;

        LgrHEADQ(const LgrHEADQ& rhs) = default;
        LgrHEADQ(LgrHEADQ&& rhs) = default;

        LgrHEADQ& operator=(const LgrHEADQ& rhs) = default;
        LgrHEADQ& operator=(LgrHEADQ&& rhs) = default;

        const std::vector<bool>& data() const
        {
            return this->data_;
        }

    private:
        std::vector<bool> data_;
    };
}} // Opm::RestartIO

#endif // OPM_LGRHEADQ_HEADER_INCLUDED
