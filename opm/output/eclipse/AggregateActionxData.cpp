/*
  Copyright 2018 Statoil ASA

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

#include <opm/output/eclipse/AggregateActionxData.hpp>

#include <opm/output/eclipse/VectorItems/action.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <opm/input/eclipse/Schedule/Action/Actdims.hpp>
#include <opm/input/eclipse/Schedule/Action/ActionContext.hpp>
#include <opm/input/eclipse/Schedule/Action/ActionX.hpp>
#include <opm/input/eclipse/Schedule/Action/Actions.hpp>
#include <opm/input/eclipse/Schedule/Action/State.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <opm/common/utility/String.hpp>
#include <opm/common/utility/TimeService.hpp>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <fmt/format.h>

// #####################################################################
// Class Opm::RestartIO::Helpers::AggregateActionxData
// ---------------------------------------------------------------------

namespace {

    namespace iACT {

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const std::vector<int>& actDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;

            int nwin = std::max(actDims[0], 1);
            int nitPrWin = std::max(actDims[1], 1);
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(nwin) },
                WV::WindowSize{ static_cast<std::size_t>(nitPrWin) }
            };
        }

        template <class IACTArray>
        void staticContrib(const Opm::Action::ActionX& actx,
                           const Opm::Action::State&   action_state,
                           IACTArray&                  iAct)
        {
            //item [0]: is unknown, (=0)
            iAct[0] = 0;
            //item [1]: The number of lines of schedule data including ENDACTIO
            iAct[1] = actx.keyword_strings().size();
            //item [2]: is the number of times an action has been triggered plus 1
            iAct[2] = action_state.run_count(actx) + 1;
            //item [3]: is unknown, (=7)
            iAct[3] = 7;
            //item [4]: is unknown, (=0)
            iAct[4] = 0;
            //item [5]: The number of times the action is triggered
            iAct[5] = actx.max_run();
            //item [6]: is unknown, (=0)
            iAct[6] = 0;
            //item [7]: is unknown, (=0)
            iAct[7] = 0;
            //item [8]: The number of conditions in an ACTIONX keyword
            iAct[8] = actx.conditions().size();
        }

    } // iAct

        namespace sACT {

        Opm::RestartIO::Helpers::WindowedArray<float>
        allocate(const std::vector<int>& actDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<float>;

            int nwin = std::max(actDims[0], 1);
            int nitPrWin = std::max(actDims[2], 1);
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(nwin) },
                WV::WindowSize{ static_cast<std::size_t>(nitPrWin) }
            };
        }

        template <class SACTArray>
        void staticContrib(const Opm::Action::ActionX& actx,
                           const Opm::Action::State& state,
                           std::time_t start_time,
                           const Opm::UnitSystem& units,
                           SACTArray& sAct)
        {
            using M  = ::Opm::UnitSystem::measure;
            sAct[0] = 0.;
            sAct[1] = 0.;
            sAct[2] = 0.;
            //item [3]:  Minimum time interval between action triggers.
            sAct[3] = units.from_si(M::time, actx.min_wait());
            //item [4]:  last time that the action was triggered
            sAct[4] =  (state.run_count(actx) > 0) ? units.from_si(M::time, (state.run_time(actx) - start_time)) : 0.;
        }

    } // sAct

    namespace zACT {

        Opm::RestartIO::Helpers::WindowedArray<
            Opm::EclIO::PaddedOutputString<8>
        >
        allocate(const std::vector<int>& actDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<
                Opm::EclIO::PaddedOutputString<8>
            >;

            int nwin = std::max(actDims[0], 1);
            int nitPrWin = std::max(actDims[3], 1);
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(nwin) },
                WV::WindowSize{ static_cast<std::size_t>(nitPrWin) }
            };
        }

    template <class ZACTArray>
    void staticContrib(const Opm::Action::ActionX& actx, ZACTArray& zAct)
    {
        // entry 1 is udq keyword
        zAct[0] = actx.name();
    }
    } // zAct

    namespace zLACT {

        Opm::RestartIO::Helpers::WindowedArray<
            Opm::EclIO::PaddedOutputString<8>
        >
        allocate(const std::size_t   num_actions,
                 const std::size_t   max_input_lines,
                 const Opm::Actdims& actdims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<
                Opm::EclIO::PaddedOutputString<8>
            >;

            return WV {
                WV::NumWindows{ num_actions },
                WV::WindowSize{ actdims.line_size() * max_input_lines }
            };
        }

        template <class ZLACTArray>
        void staticContrib(const Opm::Action::ActionX& actx,
                           const Opm::Actdims&         actdims,
                           ZLACTArray&                 zLact)
        {
            std::size_t offset = 0;
            int l_sstr = 8;
            // write out the schedule input lines
            for (auto input_line : actx.keyword_strings()) {
                input_line = Opm::trim_copy(input_line);
                if (input_line.size() > Opm::RestartIO::Helpers::VectorItems::ZLACT::max_line_length)
                    throw std::invalid_argument(fmt::format("Actionx line to long for action {}", actx.name()));

                int n_sstr =  input_line.size()/l_sstr;
                for (int i = 0; i < n_sstr; i++) {
                    zLact[offset + i] = input_line.substr(i*l_sstr, l_sstr);
                }
                //add remainder of last non-zero string
                if ((input_line.size() % l_sstr) > 0)
                    zLact[offset + n_sstr] = input_line.substr(n_sstr*l_sstr);

                offset += actdims.line_size();
            }
        }
    } // zLact

    namespace ConditionHelpers {

        template <typename T>
        class Array
        {
        public:
            using Matrix = Opm::RestartIO::Helpers::WindowedMatrix<T>;

            explicit Array(Matrix& cond, const Matrix::Idx actionID)
                : cond_   { std::ref(cond) }
                , action_ { actionID }
            {}

            decltype(auto) operator[](const Matrix::Idx condition)
            {
                return this->cond_.get()(this->action_, condition);
            }

        private:
            std::reference_wrapper<Matrix> cond_;
            Matrix::Idx action_;
        };

    } // namespace ConditionHelpers

    namespace iACN {

        std::optional<int> asInteger(std::string_view s)
        {
            auto i = -1;
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), i);

            if (ec == std::errc{}) {
                return { i };
            }
            else {
                return {};
            }
        }

        // -------------------------------------------------------------

        template <typename IACNArray>
        void captureIJKLocation(const std::array<Opm::RestartIO::Helpers::VectorItems::IACN::index, 3>& ijk,
                                const std::size_t                                                       offset,
                                const Opm::Action::Quantity&                                            quant,
                                IACNArray&                                                              iacn)
        {
            const auto& args = quant.args();

            for (auto ix = 0*ijk.size(); ix != ijk.size(); ++ix) {
                if (const auto ijkValue = asInteger(args[offset + ix]);
                    ijkValue.has_value())
                {
                    iacn[ijk[ix]] = *ijkValue;
                }
            }
        }

        template <typename IACNArray>
        void captureRegionQuantity(const Opm::RestartIO::Helpers::VectorItems::IACN::index regID,
                                   const Opm::Action::Quantity&                            quant,
                                   IACNArray&                                              iacn)
        {
            // Condition references vector of the form
            //
            //    ROPT_ABC 42
            //
            // This function assumes that quant.args holds the string "42"
            // and parses it as an integer in that case.

            if (quant.args().size() != 1) {
                return;
            }

            if (const auto regIDValue = asInteger(quant.args().front());
                regIDValue.has_value())
            {
                iacn[regID] = *regIDValue;
            }
        }

        template <typename IACNArray>
        void captureSegmentQuantity(const Opm::RestartIO::Helpers::VectorItems::IACN::index segmentID,
                                    const Opm::Action::Quantity&                            quant,
                                    IACNArray&                                              iacn)
        {
            // Condition references vector of the form
            //
            //    SOFR 'P-52' 11
            //
            // This function assumes that quant.args holds both of the
            // strings "P-52" and "11" and parses the segment number (i.e.,
            // "11") as an integer in that case.

            if (quant.args().size() != 2) {
                return;
            }

            if (const auto segmentIDValue = asInteger(quant.args()[1]);
                segmentIDValue.has_value())
            {
                iacn[segmentID] = *segmentIDValue;
            }
        }

        template <typename IACNArray>
        void captureConnectionQuantity(const std::array<Opm::RestartIO::Helpers::VectorItems::IACN::index, 3>& ijk,
                                       const Opm::Action::Quantity&                                            quant,
                                       IACNArray&                                                              iacn)
        {
            // Condition references vector of the form
            //
            //    COPR 'P-52' 11 22 33
            //
            // This function assumes that quant.args holds all of the
            // strings "P-52", "11", "22", and "33" and parses the IJK
            // indices (i.e., "11", "22", "33") as an integers in that case.

            if (quant.args().size() != 4) {
                return;
            }

            captureIJKLocation(ijk, /* args.quant.begin() + */ 1, quant, iacn);
        }

        template <typename IACNArray>
        void captureBlockQuantity(const std::array<Opm::RestartIO::Helpers::VectorItems::IACN::index, 3>& ijk,
                                  const Opm::Action::Quantity&                                            quant,
                                  IACNArray&                                                              iacn)
        {
            // Condition references vector of the form
            //
            //    BPR 11 22 33
            //
            // This function assumes that quant.args holds all of the
            // strings "11", "22", and "33" and parses the IJK indices as an
            // integers in that case.

            if (quant.args().size() != 3) {
                return;
            }

            captureIJKLocation(ijk, /* args.quant.begin() + */ 0, quant, iacn);
        }

        // -------------------------------------------------------------

        template <typename IACNArray>
        void captureLHSQuantity(const Opm::Action::Quantity& quant,
                                IACNArray&                   iacn)
        {
            using Ix = Opm::RestartIO::Helpers::VectorItems::IACN::index;
            using QType = Opm::RestartIO::Helpers::VectorItems::IACN::Value::QuantityType;

            switch ((iacn[Ix::LHSQuantityType] = quant.int_type())) {
            case QType::Region:
                captureRegionQuantity(Ix::LHSRegionID, quant, iacn);
                break;

            case QType::Segment:
                captureSegmentQuantity(Ix::LHSSegmentID, quant, iacn);
                break;

            case QType::Connection:
                captureConnectionQuantity({
                        Ix::LHSConnLocation_I,
                        Ix::LHSConnLocation_J,
                        Ix::LHSConnLocation_K,
                    }, quant, iacn);
                break;

            case QType::Block:
                captureBlockQuantity({
                        Ix::LHSBlockLocation_I,
                        Ix::LHSBlockLocation_J,
                        Ix::LHSBlockLocation_K,
                    }, quant, iacn);

            case QType::Field: case QType::Well : case QType::Group:
            case QType::Day  : case QType::Month: case QType::Year:
                // No special integer handling needed.
                break;
            }
        }

        template <typename IACNArray>
        void captureRHSQuantity(const Opm::Action::Quantity& quant,
                                IACNArray&                   iacn)
        {
            using Ix = Opm::RestartIO::Helpers::VectorItems::IACN::index;
            using QType = Opm::RestartIO::Helpers::VectorItems::IACN::Value::QuantityType;

            switch ((iacn[Ix::RHSQuantityType] = quant.int_type())) {
            case QType::Region:
                captureRegionQuantity(Ix::RHSRegionID, quant, iacn);
                break;

            case QType::Segment:
                captureSegmentQuantity(Ix::RHSSegmentID, quant, iacn);
                break;

            case QType::Connection:
                captureConnectionQuantity({
                        Ix::RHSConnLocation_I,
                        Ix::RHSConnLocation_J,
                        Ix::RHSConnLocation_K,
                    }, quant, iacn);
                break;

            case QType::Block:
                captureBlockQuantity({
                        Ix::RHSBlockLocation_I,
                        Ix::RHSBlockLocation_J,
                        Ix::RHSBlockLocation_K,
                    }, quant, iacn);

            case QType::Field: case QType::Well : case QType::Group:
            case QType::Day  : case QType::Month: case QType::Year:
                // No special integer handling needed.
                break;
            }
        }

        // item [17] - is an item that is non-zero for actions with
        // several conditions combined using logical operators (AND/OR)
        //
        //   * First condition => [17] =  0
        //   * Second+ conditions
        //
        //   *Case - no parentheses
        //       *If all conditions before current condition has AND => [17] = 1
        //       *If one condition before current condition has OR   => [17] = 0
        //
        //   *Case - parenthesis before first condition
        //       *If inside first parenthesis
        //           *If all conditions before current condition has AND [17] => 1
        //           *If one condition before current condition has OR  [17] => 0
        //       *If after first parenthesis end and outside parenthesis
        //           *If all conditions outside parentheses and before current condition has AND => [17] = 1
        //           *If one condition outside parentheses and before current condition has OR [17] => 0
        //       *If after first parenthesis end and inside a susequent parenthesis
        //           * [17] = 0
        //
        //   *Case - parenthesis after first condition
        //       *If inside parentesis => [17] = 0
        //       *If outside parenthesis:
        //           *If all conditions outside parentheses and before current condition has AND => [17] = 1
        //           *If one condition outside parentheses and before current condition has OR [17] => 0

        void inferConditionLink(const Opm::Action::ActionX&   actx,
                                ConditionHelpers::Array<int>& iAcn)
        {
            using Ix = Opm::RestartIO::Helpers::VectorItems::IACN::index;

            bool insideParen = false;
            bool parenFirstCond = false;
            bool allPrevLogicOp_AND = false;

            auto condIdx = ConditionHelpers::Array<int>::Matrix::Idx{};

            for (const auto& condition : actx.conditions()) {
                if (condIdx == 0) {
                    if (condition.open_paren()) {
                        parenFirstCond = true;
                        insideParen = true;
                    }

                    if (condition.logic == Opm::Action::Logical::AND) {
                        allPrevLogicOp_AND = true;
                    }
                }
                else {
                    // update inside parenthesis or not plus whether in
                    // first parenthesis or not
                    if (condition.open_paren()) {
                        insideParen = true;
                        parenFirstCond = false;
                    }
                    else if (condition.close_paren()) {
                        insideParen = false;
                        parenFirstCond = false;
                    }

                    // Assign [17] based on logic (see above)
                    if (allPrevLogicOp_AND &&
                        (parenFirstCond || (!parenFirstCond && !insideParen)))
                    {
                        iAcn[condIdx][Ix::BoolLink] = 1;
                    }
                    else {
                        iAcn[condIdx][Ix::BoolLink] = 0;
                    }

                    // update the previous logic-sequence
                    if (parenFirstCond && condition.logic == Opm::Action::Logical::OR) {
                        allPrevLogicOp_AND = false;
                    }
                    else if (!insideParen && condition.logic == Opm::Action::Logical::OR) {
                        allPrevLogicOp_AND = false;
                    }
                }

                ++condIdx;
            }
        }

        // -------------------------------------------------------------

        Opm::RestartIO::Helpers::WindowedMatrix<int>
        allocate(const std::size_t num_actions, const Opm::Actdims& actdims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedMatrix<int>;

            return WV {
                WV::NumRows    { num_actions },
                WV::NumCols    { actdims.max_conditions() },
                WV::WindowSize { Opm::RestartIO::Helpers::VectorItems::IACN::ConditionSize }
            };
        }

        void staticContrib(const Opm::Action::ActionX&   actx,
                           ConditionHelpers::Array<int>& iAcn)
        {
            using Ix = Opm::RestartIO::Helpers::VectorItems::IACN::index;

            if (actx.conditions().empty()) {
                // Unconditional action.  Triggers at every time step.  Unusual.
                return;
            }

            const auto first_greater = static_cast<int>
                (actx.conditions().front().cmp == Opm::Action::Comparator::LESS);

            for (auto condIdx = ConditionHelpers::Array<int>::Matrix::Idx{};
                 const auto& condition : actx.conditions())
            {
                auto iacn = iAcn[condIdx++];

                iacn[Ix::FirstGreater]  = first_greater;
                iacn[Ix::TerminalLogic] = condition.logic_as_int();
                iacn[Ix::Paren]         = condition.paren_as_int();
                iacn[Ix::Comparator]    = condition.comparator_as_int();

                captureLHSQuantity(condition.lhs, iacn);
                captureRHSQuantity(condition.rhs, iacn);
            }

            inferConditionLink(actx, iAcn);
        }

    } // namespace iAcn

    namespace sACN {

        using Ix = Opm::RestartIO::Helpers::VectorItems::SACN::index;
        using QType = Opm::RestartIO::Helpers::VectorItems::IACN::Value::QuantityType;

        // -------------------------------------------------------------

        template <typename SACNArray>
        void captureLHSDateQuantity(const double undef, SACNArray& sacn)
        {
            // Note: This function *also* overwrites 'RHS'!
            sacn[Ix::LHSValue1] = sacn[Ix::RHSValue1] = undef;
            sacn[Ix::LHSValue2] = sacn[Ix::RHSValue2] = undef;
            sacn[Ix::LHSValue3] = sacn[Ix::RHSValue3] = undef;
        }

        template <typename SACNArray>
        void captureLHSFieldQuantity(const Opm::Action::Quantity& quant,
                                     const Opm::SummaryState&     st,
                                     SACNArray&                   sacn)
        {
            const auto& lhsQuant = quant.quantity();

            if (st.has(lhsQuant)) {
                sacn[Ix::LHSValue1] = sacn[Ix::LHSValue2] = sacn[Ix::LHSValue3]
                    = st.get(lhsQuant);
            }
        }

        template <typename SACNArray>
        void captureLHSWellQuantity(const Opm::Action::Quantity& quant,
                                    std::span<const std::string> wells,
                                    const Opm::Action::Result&   ar,
                                    const Opm::SummaryState&     st,
                                    SACNArray&                   sacn)
        {
            if (! ar.conditionSatisfied()) {
                return;
            }

            // Find the well that violates action if relevant
            auto well_iter = std::ranges::find_if
                (wells,
                 [&matchSet = ar.matches()](const std::string& well)
                 { return matchSet.hasWell(well); });

            if (well_iter == wells.end()) {
                return;
            }

            const auto& lhsQuant = quant.quantity();
            const auto& wname    = *well_iter;

            if (st.has_well_var(wname, lhsQuant)) {
                sacn[Ix::LHSValue1] = sacn[Ix::LHSValue2] = sacn[Ix::LHSValue3]
                    = st.get_well_var(wname, lhsQuant);
            }
        }

        template <typename SACNArray>
        void captureLHSGroupQuantity(const Opm::Action::Quantity& quant,
                                     const Opm::SummaryState&     st,
                                     SACNArray&                   sacn)
        {
            const auto& lhsQuant = quant.quantity();
            const auto& gnames   = quant.args();

            if (! gnames.empty() && st.has_group_var(gnames.front(), lhsQuant)) {
                sacn[Ix::LHSValue1] = sacn[Ix::LHSValue2] = sacn[Ix::LHSValue3]
                    = st.get_group_var(gnames.front(), lhsQuant);
            }
        }

        // -------------------------------------------------------------

        template <typename SACNArray>
        void captureRHSConstantQuantity(const Opm::Action::Quantity& quant,
                                        SACNArray&                   sacn)
        {
            const auto& monthIx   = Opm::TimeService::eclipseMonthIndices();
            const auto& rhsQuant  = quant.quantity();
            const auto monthIxPos = monthIx.find(rhsQuant);

            const auto t_val = (monthIxPos == monthIx.end())
                ? std::stod(rhsQuant) // Numeric day, month (!), or year.
                : static_cast<double>(monthIxPos->second); // Named month.

            sacn[Ix::RHSValue0]
                = sacn[Ix::RHSValue1]
                = sacn[Ix::RHSValue2]
                = sacn[Ix::RHSValue3]
                = t_val;
        }

        template <typename SACNArray>
        void captureRHSFieldQuantity(const Opm::Action::Quantity& quant,
                                     const Opm::SummaryState&     st,
                                     SACNArray&                   sacn)
        {
            const auto& rhsQuant = quant.quantity();

            if (st.has(rhsQuant)) {
                sacn[Ix::RHSValue1] = sacn[Ix::RHSValue2] = sacn[Ix::RHSValue3]
                    = st.get(rhsQuant);
            }
        }

        template <typename SACNArray>
        void captureRHSWellQuantity(const Opm::Action::Quantity& quant,
                                    const Opm::SummaryState&     st,
                                    SACNArray&                   sacn)
        {
            const auto& rhsQuant = quant.quantity();
            const auto& wnames   = quant.args();

            if (! wnames.empty() && st.has_well_var(wnames.front(), rhsQuant)) {
                sacn[Ix::RHSValue1] = sacn[Ix::RHSValue2] = sacn[Ix::RHSValue3]
                    = st.get_well_var(wnames.front(), rhsQuant);
            }
        }

        template <typename SACNArray>
        void captureRHSGroupQuantity(const Opm::Action::Quantity& quant,
                                     const Opm::SummaryState&     st,
                                     SACNArray&                   sacn)
        {
            const auto& rhsQuant = quant.quantity();
            const auto& gnames   = quant.args();

            if (! gnames.empty() && st.has_group_var(gnames.front(), rhsQuant)) {
                sacn[Ix::RHSValue1] = sacn[Ix::RHSValue2] = sacn[Ix::RHSValue3]
                    = st.get_group_var(gnames.front(), rhsQuant);
            }
        }

        // -------------------------------------------------------------

        template <typename SACNArray>
        void captureLHSQuantity(const Opm::Action::Quantity& quant,
                                std::span<const std::string> wells,
                                const Opm::Action::Result&   ar,
                                const Opm::SummaryState&     st,
                                const double                 undef,
                                SACNArray&                   sacn)
        {
            switch (quant.int_type()) {
            case QType::Field:
                captureLHSFieldQuantity(quant, st, sacn);
                break;

            case QType::Well:
                captureLHSWellQuantity(quant, wells, ar, st, sacn);
                break;

            case QType::Group:
                captureLHSGroupQuantity(quant, st, sacn);
                break;

            case QType::Day:
            case QType::Month:
            case QType::Year:
                captureLHSDateQuantity(undef, sacn);
                break;

            case QType::Region:
            case QType::Segment:
            case QType::Connection:
            case QType::Const:
            case QType::Block:
                // Unhandled.
                break;
            }
        }

        template <typename SACNArray>
        void captureRHSQuantity(const Opm::Action::Quantity& quant,
                                const Opm::SummaryState&     st,
                                SACNArray&                   sacn)
        {
            switch (quant.int_type()) {
            case QType::Field:
                captureRHSFieldQuantity(quant, st, sacn);
                break;

            case QType::Well:
                captureRHSWellQuantity(quant, st, sacn);
                break;

            case QType::Group:
                captureRHSGroupQuantity(quant, st, sacn);
                break;

            case QType::Const:
            case QType::Day:
            case QType::Month:
            case QType::Year:
                captureRHSConstantQuantity(quant, sacn);

            case QType::Region:
            case QType::Segment:
            case QType::Connection:
            case QType::Block:
                // Unhandled.
                break;
            }
        }

        Opm::Action::Result
        act_res(const Opm::Schedule&        sched,
                const Opm::Action::State&   action_state,
                const Opm::SummaryState&    smry,
                const std::size_t           sim_step,
                const Opm::Action::ActionX& action)
        {
            const auto sim_time = sched.simTime(sim_step);

            if (! action.ready(action_state, sim_time)) {
                return Opm::Action::Result { false };
            }

            const auto context = Opm::Action::Context {
                smry, sched[sim_step].wlist_manager.get()
            };

            return action.eval(context);
        }

        Opm::RestartIO::Helpers::WindowedMatrix<double>
        allocate(std::size_t num_actions, const Opm::Actdims& actdims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedMatrix<double>;

            return WV {
                WV::NumRows    { num_actions },
                WV::NumCols    { actdims.max_conditions() },
                WV::WindowSize { Opm::RestartIO::Helpers::VectorItems::SACN::ConditionSize }
            };
        }

        void staticContrib(const Opm::Action::ActionX&      action,
                           const Opm::Action::State&        action_state,
                           const Opm::SummaryState&         st,
                           const Opm::Schedule&             sched,
                           const std::size_t                simStep,
                           ConditionHelpers::Array<double>& sAcn)
        {
            const auto undef_high_val = 1.0E+20;
            const auto wells = sched.wellNames(simStep);
            const auto ar = act_res(sched, action_state, st, simStep, action);

            // Write out the schedule ACTIONX conditions
            auto condIx = std::size_t{0};
            for (const auto& condition : action.conditions()) {
                auto sacn = sAcn[condIx++];

                // Note ordering peculiarity here: We *must* capture the RHS
                // quantity *before* the LHS quantity.  Certain RHS entries
                // in SACN will be overwritten/cleared if the LHS is a date
                // quantity (i.e., DAY, MNTH/MONTH, or YEAR).
                captureRHSQuantity(condition.rhs, st, sacn);

                captureLHSQuantity(condition.lhs,
                                   wells, ar, st,
                                   undef_high_val,
                                   sacn);
            }
        }

    } // namespace sAcn

    namespace zACN {

        template <typename ZACNArray>
        void captureUnnamedQuantity(const Opm::RestartIO::Helpers::VectorItems::ZACN::index quantityIx,
                                    const Opm::Action::Quantity&                            quant,
                                    ZACNArray&                                              zacn)
        {
            zacn[quantityIx] = quant.quantity();
        }

        template <typename ZACNArray>
        void captureNamedQuantity(const Opm::RestartIO::Helpers::VectorItems::ZACN::index quantityIx,
                                  const Opm::RestartIO::Helpers::VectorItems::ZACN::index nameIx,
                                  const Opm::Action::Quantity&                            quant,
                                  ZACNArray&                                              zacn)
        {
            captureUnnamedQuantity(quantityIx, quant, zacn);

            if (! quant.args().empty()) {
                zacn[nameIx] = quant.args().front();
            }
        }

        template <typename ZACNArray>
        void captureRegionQuantity(const Opm::RestartIO::Helpers::VectorItems::ZACN::index quantityIx,
                                   const Opm::RestartIO::Helpers::VectorItems::ZACN::index regsetIx,
                                   const Opm::Action::Quantity&                            quant,
                                   ZACNArray&                                              zacn)
        {
            // This code *depends* on quant.quantity() having the canonical form
            //
            //    ROPR_SET
            //
            // even when 'SET' is the built-in FIPNUM region set--i.e., ROPR_NUM.
            zacn[quantityIx] = quant.quantity();
            zacn[regsetIx]   = quant.quantity().substr(5);
        }

        template <typename ZACNArray>
        void captureLHSQuantity(const Opm::Action::Quantity& quant,
                                ZACNArray&                   zacn)
        {
            using Ix = Opm::RestartIO::Helpers::VectorItems::ZACN::index;
            using QType = Opm::RestartIO::Helpers::VectorItems::IACN::Value::QuantityType;

            switch (quant.int_type()) {
            case QType::Field:
            case QType::Block:
                captureUnnamedQuantity(Ix::LHSQuantity, quant, zacn);
                break;

            case QType::Well:
            case QType::Segment:
            case QType::Connection:
                captureNamedQuantity(Ix::LHSQuantity, Ix::LHSWell, quant, zacn);
                break;

            case QType::Group:
                captureNamedQuantity(Ix::LHSQuantity, Ix::LHSGroup, quant, zacn);
                break;

            case QType::Region:
                captureRegionQuantity(Ix::LHSQuantity, Ix::LHSRegionSet, quant, zacn);
                break;

            case QType::Day: case QType::Month: case QType::Year:
                // No special string/name handling needed.
                break;
            }
        }

        template <typename ZACNArray>
        void captureRHSQuantity(const Opm::Action::Quantity& quant,
                                ZACNArray&                   zacn)
        {
            using Ix = Opm::RestartIO::Helpers::VectorItems::ZACN::index;
            using QType = Opm::RestartIO::Helpers::VectorItems::IACN::Value::QuantityType;

            switch (quant.int_type()) {
            case QType::Field:
            case QType::Block:
                captureUnnamedQuantity(Ix::RHSQuantity, quant, zacn);
                break;

            case QType::Well:
            case QType::Segment:
            case QType::Connection:
                captureNamedQuantity(Ix::RHSQuantity, Ix::RHSWell, quant, zacn);
                break;

            case QType::Group:
                captureNamedQuantity(Ix::RHSQuantity, Ix::RHSGroup, quant, zacn);
                break;

            case QType::Region:
                captureRegionQuantity(Ix::RHSQuantity, Ix::RHSRegionSet, quant, zacn);
                break;

            case QType::Day: case QType::Month: case QType::Year:
                // No special string/name handling needed.
                break;
            }
        }

        Opm::RestartIO::Helpers::WindowedMatrix<
            Opm::EclIO::PaddedOutputString<8>
        >
        allocate(const std::size_t num_actions, const Opm::Actdims& actdims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedMatrix<
                Opm::EclIO::PaddedOutputString<8>
            >;

            return WV {
                WV::NumRows    { num_actions },
                WV::NumCols    { actdims.max_conditions() },
                WV::WindowSize { Opm::RestartIO::Helpers::VectorItems::ZACN::ConditionSize }
            };
        }

        void staticContrib(const Opm::Action::ActionX&                                 actx,
                           ConditionHelpers::Array<Opm::EclIO::PaddedOutputString<8>>& zAcn)
        {
            using Ix = Opm::RestartIO::Helpers::VectorItems::ZACN::index;

            if (actx.conditions().empty()) {
                // Unconditional action.  Triggers at every time step.  Unusual.
                return;
            }

            for (auto condIdx = ConditionHelpers::Array<int>::Matrix::Idx{};
                 const auto& condition : actx.conditions())
            {
                auto zacn = zAcn[condIdx++];

                // Comparison operator.
                zacn[Ix::Comparator] = condition.cmp_string;

                captureLHSQuantity(condition.lhs, zacn);
                captureRHSQuantity(condition.rhs, zacn);
            }
        }
    } // namespace zAcn

} // Anonymous namespace

// =====================================================================

Opm::RestartIO::Helpers::AggregateActionxData::
AggregateActionxData(const std::vector<int>&   rst_dims,
                     const std::size_t         num_actions,
                     const Opm::Actdims&       actdims,
                     const Opm::Schedule&      sched,
                     const Opm::Action::State& action_state,
                     const Opm::SummaryState&  st,
                     const std::size_t         simStep)
    : iACT_ { iACT::allocate(rst_dims) }
    , sACT_ { sACT::allocate(rst_dims) }
    , zACT_ { zACT::allocate(rst_dims) }
    , zLACT_{ zLACT::allocate(num_actions,
                              sched[simStep].actions().max_input_lines(),
                              actdims) }
    , iACN_ { iACN::allocate(num_actions, actdims) }
    , sACN_ { sACN::allocate(num_actions, actdims) }
    , zACN_ { zACN::allocate(num_actions, actdims) }
{
    std::size_t act_ind = 0;
    for (const auto& action : sched[simStep].actions()) {
        {
            auto i_act = this->iACT_[act_ind];
            iACT::staticContrib(action, action_state, i_act);
        }

        {
            auto s_act = this->sACT_[act_ind];
            sACT::staticContrib(action, action_state,
                                sched.getStartTime(),
                                sched.getUnits(),
                                s_act);
        }

        {
            auto z_act = this->zACT_[act_ind];
            zACT::staticContrib(action, z_act);
        }

        {
            auto z_lact = this->zLACT_[act_ind];
            zLACT::staticContrib(action, actdims, z_lact);
        }

        {
            auto i_acn = ConditionHelpers::Array<int> { this->iACN_, act_ind };
            iACN::staticContrib(action, i_acn);
        }

        {
            auto s_acn = ConditionHelpers::Array<double> { this->sACN_, act_ind };
            sACN::staticContrib(action, action_state, st, sched, simStep, s_acn);
        }

        {
            auto z_acn = ConditionHelpers::Array<EclIO::PaddedOutputString<8>> {
                this->zACN_, act_ind
            };

            zACN::staticContrib(action, z_acn);
        }

        ++act_ind;
    }
}

Opm::RestartIO::Helpers::AggregateActionxData::
AggregateActionxData(const Opm::Schedule&      sched,
                     const Opm::Action::State& action_state,
                     const Opm::SummaryState&  st,
                     const std::size_t         simStep)
    : AggregateActionxData { createActionRSTDims(sched, simStep),
                             sched[simStep].actions().ecl_size(),
                             sched.runspec().actdims(),
                             sched, action_state, st, simStep }
{}
