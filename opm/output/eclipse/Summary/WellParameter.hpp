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

#ifndef OPM_SUMMARY_WELLPARAMETER_HPP
#define OPM_SUMMARY_WELLPARAMETER_HPP

#include <opm/output/eclipse/Summary/EvaluateQuantity.hpp>
#include <opm/output/eclipse/Summary/SummaryParameter.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <bitset>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <utility>

namespace Opm {
    class WellParameter : public SummaryParameter
    {
    public:
        struct WellName   { std::string value; };
        struct Keyword    { std::string value; };
        struct UnitString { std::string value; };

        enum class FlowType { Rate, Total, Ratio };
        enum class Pressure { BHP, THP };

        explicit WellParameter(WellName                  wellname,
                               Keyword                   keyword,
                               UnitString                unit,
                               SummaryHelpers::Evaluator eval);

        WellParameter& flowType(const FlowType type);
        WellParameter& pressure(const Pressure type);

        const WellParameter& validate() const &;
        WellParameter validate() &&;

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

        virtual std::string unit(const UnitSystem& /* usys */) const override
        {
            return this->unit_;
        }

    private:
        enum class Flag : std::size_t {
            Rate,
            Ratio,
            Total,
            BHP,
            THP,

            // Must be last item in enumeration
            NumFlags,
        };

        using TypeFlags = std::bitset<
            static_cast<std::size_t>(Flag::NumFlags)
        >;

        std::string wellname_;
        std::string keyword_;
        std::string unit_;

        SummaryHelpers::Evaluator evalParam_;

        /// Unique summary state lookup key associating
        /// parameter keyword with particular well (name).
        std::string sumKey_;

        TypeFlags typeFlags_{};

        void setFlag(const Flag                  f,
                     std::initializer_list<Flag> conflict);

        bool isPressure() const
        {
            return this->isSet(Flag::BHP)
                || this->isSet(Flag::THP);
        }

        bool isFlow() const
        {
            return this->isSet(Flag::Rate)
                || this->isSet(Flag::Ratio)
                || this->isTotal();
        }

        bool isTotal() const
        {
            return this->isSet(Flag::Total);
        }

        bool isSet(const Flag f) const
        {
            return this->typeFlags_[static_cast<std::size_t>(f)];
        }

        bool isValidParamType() const
        {
            return this->typeFlags_.any();
        }

        std::string flagName(const Flag f) const;

        void validateCore() const;
    };

    class WellAggregateRegionParameter : public SummaryParameter
    {
    public:
        struct Keyword    { std::string value; };
        struct UnitString { std::string value; };

        enum class Type { Rate, Total };

        explicit WellAggregateRegionParameter(const int                 regionID,
                                              Keyword                   keyword,
                                              const Type                type,
                                              UnitString                unit,
                                              SummaryHelpers::Evaluator eval);

        const WellAggregateRegionParameter& validate() const &;
        WellAggregateRegionParameter validate() &&;

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
            return this->regionID_;
        }

        virtual std::string unit(const UnitSystem& /* usys */) const override
        {
            return this->unit_;
        }

    private:
        std::string keyword_;
        std::string unit_;
        int         regionID_;
        Type        type_;

        SummaryHelpers::Evaluator evalParam_;

        /// Unique summary state lookup key associating
        /// parameter keyword with particular well (name).
        std::string sumKey_;

        bool isRate() const
        {
            return this->is(Type::Rate);
        }

        bool isTotal() const
        {
            return this->is(Type::Total);
        }

        bool is(const Type t) const
        {
            return this->type_ == t;
        }

        void validateCore() const;
    };
} // namespace Opm

#endif // OPM_SUMMARY_WELLPARAMETER_HPP
