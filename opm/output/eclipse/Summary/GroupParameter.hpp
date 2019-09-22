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

#ifndef OPM_SUMMARY_GROUPPARAMETER_HPP
#define OPM_SUMMARY_GROUPPARAMETER_HPP

#include <opm/output/eclipse/Summary/EvaluateQuantity.hpp>
#include <opm/output/eclipse/Summary/SummaryParameter.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <cstddef>
#include <initializer_list>
#include <string>
#include <utility>

namespace Opm {
    class GroupParameter : public SummaryParameter
    {
    public:
        struct GroupName  { std::string value; };
        struct Keyword    { std::string value; };
        struct UnitString { std::string value; };

        enum class Type { Count, Rate, Total, Ratio };

        explicit GroupParameter(GroupName                 groupname,
                                Keyword                   keyword,
                                UnitString                unit,
                                const Type                type,
                                SummaryHelpers::Evaluator eval);

        const GroupParameter& validate() const &;
        GroupParameter validate() &&;

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
            return this->groupname_;
        }

        virtual std::string unit(const UnitSystem& /* usys */) const override
        {
            return this->unit_;
        }

    protected:
        const std::string& keywordNoCopy() const
        {
            return this->keyword_;
        }

        double parameterValue(const std::size_t       reportStep,
                              const double            stepSize,
                              const InputData&        input,
                              const SimulatorResults& simRes,
                              const SummaryState&     st) const;

        void validateCore() const;

    private:
        std::string groupname_;
        std::string keyword_;
        std::string unit_;
        Type        type_;

        SummaryHelpers::Evaluator evalParam_;

        /// Unique summary state lookup key associating
        /// parameter keyword with particular well (name).
        std::string sumKey_;

        std::vector<std::pair<std::string, double>>
        efficiencyFactors(const std::size_t sim_step,
                          const Schedule&   sched) const;

        virtual std::vector<std::string>
        wells(const std::size_t sim_step,
              const Schedule&   sched) const;

        bool isCount() const
        {
            return this->is(Type::Count);
        }

        bool isFlow() const
        {
            return this->isRate()
                || this->isRatio()
                || this->isTotal();
        }

        bool isRate() const
        {
            return this->is(Type::Rate);
        }

        bool isRatio() const
        {
            return this->is(Type::Ratio);
        }

        bool isTotal() const
        {
            return this->is(Type::Total);
        }

        bool is(const Type t) const
        {
            return this->type_ == t;
        }

        bool isValidParamType() const
        {
            return this->isCount() || this->isFlow();
        }
    };

    class FieldParameter : public GroupParameter
    {
    public:
        using GroupParameter::Keyword;
        using GroupParameter::UnitString;
        using GroupParameter::Type;

        explicit FieldParameter(Keyword                   keyword,
                                UnitString                unit,
                                const Type                type,
                                SummaryHelpers::Evaluator eval);

        virtual void update(const std::size_t       reportStep,
                            const double            stepSize,
                            const InputData&        input,
                            const SimulatorResults& simRes,
                            SummaryState&           st) const override;

        const FieldParameter& validate() const &;
        FieldParameter validate() &&;

        using GroupParameter::summaryKey;
        using GroupParameter::keyword;
        using GroupParameter::name;
        using GroupParameter::unit;

    private:
        virtual std::vector<std::string>
        wells(const std::size_t sim_step,
              const Schedule&   sched) const override;
    };
} // namespace Opm

#endif // OPM_SUMMARY_GROUPPARAMETER_HPP
