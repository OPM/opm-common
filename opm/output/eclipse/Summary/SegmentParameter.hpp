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

#ifndef OPM_SUMMARY_SEGMENTPARAMETER_HPP
#define OPM_SUMMARY_SEGMENTPARAMETER_HPP

#include <opm/output/eclipse/Summary/EvaluateQuantity.hpp>
#include <opm/output/eclipse/Summary/SummaryParameter.hpp>

#include <cstddef>
#include <string>

namespace Opm {
    class SummaryState;
}

namespace Opm {
    class SegmentParameter : public SummaryParameter
    {
    public:
        struct WellName   { std::string value; };
        struct Keyword    { std::string value; };
        struct UnitString { std::string value; };

        enum class Type { Rate, Pressure };

        SegmentParameter(WellName                  well,
                         const int                 segmentID,
                         Keyword                   keyword,
                         UnitString                unit,
                         const Type                type,
                         SummaryHelpers::Evaluator eval);

        const SegmentParameter& validate() const &;
        SegmentParameter validate() &&;

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

        virtual std::string name() const override
        {
            return this->wellname_;
        }

        virtual int num() const override
        {
            return this->segmentID_;
        }

        virtual std::string unit(const UnitSystem& /* usys */) const override
        {
            return this->unit_;
        }

    private:
        std::string wellname_;
        std::string keyword_;
        std::string unit_;
        int         segmentID_;
        Type        type_;

        SummaryHelpers::Evaluator evalParam_;

        /// Unique summary state lookup key associating
        /// parameter keyword with particular well (name).
        std::string sumKey_;

        bool isRate() const
        {
            return this->is(Type::Rate);
        }

        bool isPressure() const
        {
            return this->is(Type::Pressure);
        }

        bool is(const Type t) const
        {
            return this->type_ == t;
        }

        bool isValidParamType() const
        {
            return this->isRate() || this->isPressure();
        }

        void validateCore() const;
    };
} // namespace Opm

#endif // OPM_SUMMARY_SEGMENTPARAMETER_HPP
