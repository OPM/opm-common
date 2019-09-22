/*
  Copyright (c) 2019 Equinor ASA

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

#ifndef OPM_SUMMARY_BLOCKPARAMETER_HPP
#define OPM_SUMMARY_BLOCKPARAMETER_HPP

#include <opm/output/eclipse/Summary/SummaryParameter.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <cstddef>
#include <string>
#include <utility>

namespace Opm {
    class BlockParameter : public SummaryParameter
    {
    public:
        explicit BlockParameter(const int                 num,
                                const UnitSystem::measure m,
                                std::string               keyword);

        virtual void update(const std::size_t       reportStep,
                            const double            stepSize,
                            const InputData&        input,
                            const SimulatorResults& simRes,
                            SummaryState&           st) const override;

        virtual std::string summaryKey() const override
        {
            return this->sumKey_;
        }

        virtual std::string keyword() const override
        {
            return this->keyword_;
        }

        virtual int num() const override
        {
            return this->num_;
        }

        virtual std::string unit(const UnitSystem& usys) const override
        {
            return usys.name(this->m_);
        }

    private:
        /// Global (Cartesian) index of pertinent block (cell).
        int num_;

        /// Unit of measure for this summary parameter.
        UnitSystem::measure m_;

        /// Identifying keyword of this summary parameter.
        std::string keyword_;

        /// Unique summary state lookup key associating
        /// parameter keyword with block ID.
        std::string sumKey_;
    };
} // namespace Opm

#endif // OPM_SUMMARY_BLOCKPARAMETER_HPP
