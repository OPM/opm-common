/*
  Copyright (c) 2018 Statoil ASA

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

#ifndef OPM_AGGREGATE_UDQ_DATA_HPP
#define OPM_AGGREGATE_UDQ_DATA_HPP

#include <opm/output/eclipse/WindowedArray.hpp>

#include <opm/io/eclipse/PaddedOutputString.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace Opm {
    class Group;
    class Schedule;
    class UDQConfig;
    class UDQDims;
    class UDQInput;
    class UDQState;
} // Opm

namespace Opm::RestartIO::Helpers {

class AggregateUDQData
{
public:
    explicit AggregateUDQData(const UDQDims& udqDims);

    void captureDeclaredUDQData(const Schedule&         sched,
                                const std::size_t       simStep,
                                const UDQState&         udqState,
                                const std::vector<int>& inteHead);

    const std::vector<int>& getIUDQ() const
    {
        return this->iUDQ_.data();
    }

    const std::vector<int>& getIUAD() const
    {
        return this->iUAD_.data();
    }

    const std::vector<EclIO::PaddedOutputString<8>>& getZUDN() const
    {
        return this->zUDN_.data();
    }

    const std::vector<EclIO::PaddedOutputString<8>>& getZUDL() const
    {
        return this->zUDL_.data();
    }

    const std::vector<int>& getIGPH() const
    {
        return this->iGPH_.data();
    }

    const std::vector<int>& getIUAP() const
    {
        return this->iUAP_.data();
    }

    /// Retrieve values of field level UDQs.  Nullopt if no such UDQs exist.
    const std::optional<WindowedArray<double>>& getDUDF() const
    {
        return this->dUDF_;
    }

    /// Retrieve values of group level UDQs.  Nullopt if no such UDQs exist.
    const std::optional<WindowedArray<double>>& getDUDG() const
    {
        return this->dUDG_;
    }

    /// Retrieve values of well level UDQs.  Nullopt if no such UDQs exist.
    const std::optional<WindowedArray<double>>& getDUDW() const
    {
        return this->dUDW_;
    }

private:
    /// Aggregate 'IUDQ' array (Integer) for all UDQ data
    ///
    /// 3 integers pr UDQ.
    WindowedArray<int> iUDQ_;

    /// Aggregate 'IUAD' array (Integer) for all UDQ data
    ///
    /// 5 integers pr UDQ that is used for various well and group controls.
    WindowedArray<int> iUAD_;

    /// Aggregate 'ZUDN' array (Character) for all UDQ data.
    ///
    /// 2 * 8 chars pr UDQ -> UNIT keyword.
    WindowedArray<EclIO::PaddedOutputString<8>> zUDN_;

    /// Aggregate 'ZUDL' array (Character) for all UDQ data.
    ///
    /// 16 * 8 chars pr UDQ DEFINE, Data for operation - Math Expression
    WindowedArray<EclIO::PaddedOutputString<8>> zUDL_;

    /// Aggregate 'IGPH' array (Integer) for all UDQ data
    ///
    /// 3 - zeroes - as of current understanding
    WindowedArray<int> iGPH_;

    /// Aggregate 'IUAP' array for all UDQ data
    ///
    /// 1 integer pr UDQ constraint used
    WindowedArray<int> iUAP_;

    /// Numeric values of field level UDQs.
    ///
    /// Nullopt if no such UDQs exist, number of field level UDQs otherwise.
    std::optional<WindowedArray<double>> dUDF_{};

    /// Numeric values of group level UDQs.
    ///
    /// Nullopt if no such UDQs exist, declared maximum #groups + 1 elements
    /// for each group level UDQ otherwise.
    std::optional<WindowedArray<double>> dUDG_{};

    /// Numeric values of well level UDQs.
    ///
    /// Nullopt if no such UDQs exist, declared maximum #wells elements for
    /// each well level UDQ otherwise.
    std::optional<WindowedArray<double>> dUDW_{};

    void collectUserDefinedQuantities(const std::vector<UDQInput>& udqInput,
                                      const std::vector<int>&      inteHead);

    void collectUserDefinedArguments(const Schedule&         sched,
                                     const std::size_t       simStep,
                                     const std::vector<int>& inteHead);

    void collectFieldUDQValues(const std::vector<UDQInput>& udqInput,
                               const UDQState&              udq_state,
                               const int                    expectNumFieldUDQs);

    void collectGroupUDQValues(const std::vector<UDQInput>&     udqInput,
                               const UDQState&                  udqState,
                               const std::size_t                ngmax,
                               const std::vector<const Group*>& groups,
                               const int                        expectedNumGroupUDQs);

    void collectWellUDQValues(const std::vector<UDQInput>&    udqInput,
                              const UDQState&                 udqState,
                              const std::size_t               nwmax,
                              const std::vector<std::string>& wells,
                              const int                       expectedNumWellUDQs);
};

} // Opm::RestartIO::Helpers

#endif // OPM_AGGREGATE_UDQ_DATA_HPP
