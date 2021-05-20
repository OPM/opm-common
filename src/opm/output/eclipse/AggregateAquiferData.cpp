/*
  Copyright (c) 2021 Equinor ASA

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

#include <opm/output/eclipse/AggregateAquiferData.hpp>

#include <opm/output/eclipse/InteHEAD.hpp>
#include <opm/output/eclipse/WindowedArray.hpp>
#include <opm/output/eclipse/ActiveIndexByColumns.hpp>

#include <opm/output/eclipse/VectorItems/aquifer.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Aquifer/Aquancon.hpp>
#include <opm/parser/eclipse/EclipseState/Aquifer/AquiferConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Aquifer/AquiferCT.hpp>
#include <opm/parser/eclipse/EclipseState/Aquifer/Aquifetp.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>

#include <opm/parser/eclipse/EclipseState/Tables/FlatTable.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <fmt/format.h>

#include <array>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace VI = Opm::RestartIO::Helpers::VectorItems;

namespace {
    double getSummaryVariable(const Opm::SummaryState& summaryState,
                              const std::string&       variable,
                              const int                aquiferID)
    {
        const auto key = fmt::format("{}:{}", variable, aquiferID);

        return summaryState.get(key, 0.0);
    }

    template <typename AquiferCallBack>
    void CarterTracyAquiferLoop(const Opm::AquiferConfig& aqConfig,
                                AquiferCallBack&&         aquiferOp)
    {
        for (const auto& aqData : aqConfig.ct()) {
            aquiferOp(aqData);
        }
    }

    template <typename AquiferCallBack>
    void FetkovichAquiferLoop(const Opm::AquiferConfig& aqConfig,
                              AquiferCallBack&&         aquiferOp)
    {
        for (const auto& aqData : aqConfig.fetp()) {
            aquiferOp(aqData);
        }
    }

    template <typename AquiferCallBack>
    void numericAquiferLoop(const Opm::AquiferConfig& aqConfig,
                            AquiferCallBack&&         aquiferOp)
    {
        if (! aqConfig.hasNumericalAquifer()) {
            return;
        }

        for (const auto& [aquiferID, aquifer] : aqConfig.numericalAquifers().aquifers()) {
            const auto numCells = aquifer.numCells();

            for (auto cellIndex = 0*numCells; cellIndex < numCells; ++cellIndex) {
                const auto* aqCell = aquifer.getCellPrt(cellIndex);
                aquiferOp(static_cast<int>(aquiferID), cellIndex == 0*numCells, *aqCell);
            }
        }
    }

    template <typename ConnectionCallBack>
    void analyticAquiferConnectionLoop(const Opm::AquiferConfig& aqConfig,
                                       ConnectionCallBack&&      connectionOp)
    {
        for (const auto& [aquiferID, connections] : aqConfig.connections().data()) {
            const auto tot_influx =
                std::accumulate(connections.begin(), connections.end(), 0.0,
                    [](const double t, const Opm::Aquancon::AquancCell& connection) -> double
                {
                    return t + connection.influx_coeff;
                });

            auto connectionID = std::size_t{0};

            for (const auto& connection : connections) {
                connectionOp(aquiferID, connectionID++, tot_influx, connection);
            }
        }
    }

    namespace IntegerAnalyticAquifer
    {
        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const Opm::RestartIO::InteHEAD::AquiferDims& aqDims)
        {
            using WA = Opm::RestartIO::Helpers::WindowedArray<int>;

            return WA {
                WA::NumWindows{ static_cast<WA::Idx>(aqDims.maxAquiferID) },
                WA::WindowSize{ static_cast<WA::Idx>(aqDims.numIntAquiferElem) }
            };
        }

        namespace CarterTracy
        {
            template <typename IAaqArray>
            void staticContrib(const Opm::AquiferCT::AQUCT_data& aquifer,
                               const int                         numActiveConn,
                               IAaqArray&                        iaaq)
            {
                using Ix = VI::IAnalyticAquifer::index;

                iaaq[Ix::NumAquiferConn] = numActiveConn;
                iaaq[Ix::WatPropTable] = aquifer.pvttableID; // One-based (=AQUCT(10))

                iaaq[Ix::CTInfluenceFunction] = aquifer.inftableID;
                iaaq[Ix::TypeRelated1] = VI::IAnalyticAquifer::Value::ModelType::CarterTracy;

                iaaq[Ix::Unknown_1] = 1; // Not characterised; =1 in all cases seen thus far.
            }
        } // CarterTracy

        namespace Fetckovich
        {
            template <typename IAaqArray>
            void staticContrib(const Opm::Aquifetp::AQUFETP_data& aquifer,
                               const int                          numActiveConn,
                               IAaqArray&                         iaaq)
            {
                using Ix = VI::IAnalyticAquifer::index;

                iaaq[Ix::NumAquiferConn] = numActiveConn;
                iaaq[Ix::WatPropTable] = aquifer.pvttableID; // One-based (=AQUFETP(7))

                iaaq[Ix::TypeRelated1] = VI::IAnalyticAquifer::Value::ModelType::Fetkovich;
                iaaq[Ix::Unknown_1] = 1; // Not characterised; =1 in all cases seen thus far.
            }
        } // Fetckovich
    } // IntegerAnalyticAquifer

    namespace IntegerNumericAquifer
    {
        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const Opm::RestartIO::InteHEAD::AquiferDims& aqDims)
        {
            using WA = Opm::RestartIO::Helpers::WindowedArray<int>;

            return WA {
                WA::NumWindows{ static_cast<WA::Idx>(aqDims.numNumericAquiferRecords) },
                WA::WindowSize{ static_cast<WA::Idx>(aqDims.numNumericAquiferIntElem) }
            };
        }

        template <typename IAqnArray>
        void staticContrib(const Opm::NumericalAquiferCell& aqCell,
                           const int                        aquiferID,
                           IAqnArray&                       iaqn)
        {
            using Ix = VI::INumericAquifer::index;

            iaqn[Ix::AquiferID] = aquiferID;

            iaqn[Ix::Cell_I] = static_cast<int>(aqCell.I) + 1;
            iaqn[Ix::Cell_J] = static_cast<int>(aqCell.J) + 1;
            iaqn[Ix::Cell_K] = static_cast<int>(aqCell.K) + 1;

            iaqn[Ix::PVTTableID] = aqCell.pvttable;
            iaqn[Ix::SatFuncID] = aqCell.sattable;
        }
    } // IntegerNumericAquifer

    namespace IntegerAnalyticAquiferConn
    {
        std::vector<Opm::RestartIO::Helpers::WindowedArray<int>>
        allocate(const Opm::RestartIO::InteHEAD::AquiferDims& aqDims)
        {
            using WA = Opm::RestartIO::Helpers::WindowedArray<int>;

            return std::vector<WA>(aqDims.maxAquiferID, WA {
                WA::NumWindows{ static_cast<WA::Idx>(aqDims.maxNumActiveAquiferConn) },
                WA::WindowSize{ static_cast<WA::Idx>(aqDims.numIntConnElem) }
            });
        }

        int eclipseFaceDirection(const Opm::FaceDir::DirEnum faceDir)
        {
            using FDValue = VI::IAnalyticAquiferConn::Value::FaceDirection;

            switch (faceDir) {
            case Opm::FaceDir::DirEnum::XMinus: return FDValue::IMinus;
            case Opm::FaceDir::DirEnum::XPlus:  return FDValue::IPlus;
            case Opm::FaceDir::DirEnum::YMinus: return FDValue::JMinus;
            case Opm::FaceDir::DirEnum::YPlus:  return FDValue::JPlus;
            case Opm::FaceDir::DirEnum::ZMinus: return FDValue::KMinus;
            case Opm::FaceDir::DirEnum::ZPlus:  return FDValue::KPlus;
            }

            throw std::invalid_argument {
                fmt::format("Unknown Face Direction {}", static_cast<int>(faceDir))
            };
        }

        template <typename ICAqArray>
        void staticContrib(const Opm::Aquancon::AquancCell& connection,
                           const Opm::EclipseGrid&          grid,
                           const Opm::ActiveIndexByColumns& map,
                           ICAqArray&                       icaq)
        {
            using Ix = VI::IAnalyticAquiferConn::index;

            const auto ijk = grid.getIJK(connection.global_index);
            icaq[Ix::Index_I] = ijk[0] + 1;
            icaq[Ix::Index_J] = ijk[1] + 1;
            icaq[Ix::Index_K] = ijk[2] + 1;

            icaq[Ix::ActiveIndex] = map.getColumnarActiveIndex(grid.activeIndex(connection.global_index)) + 1;

            icaq[Ix::FaceDirection] = eclipseFaceDirection(connection.face_dir);
        }
    } // IntegerAnalyticAquiferConn

    namespace SinglePrecAnalyticAquifer
    {
        Opm::RestartIO::Helpers::WindowedArray<float>
        allocate(const Opm::RestartIO::InteHEAD::AquiferDims& aqDims)
        {
            using WA = Opm::RestartIO::Helpers::WindowedArray<float>;

            return WA {
                WA::NumWindows{ static_cast<WA::Idx>(aqDims.maxAquiferID) },
                WA::WindowSize{ static_cast<WA::Idx>(aqDims.numRealAquiferElem) }
            };
        }

        namespace CarterTracy {
            template <typename SAaqArray>
            void staticContrib(const Opm::AquiferCT::AQUCT_data& aquifer,
                               const double                      rhoWS,
                               const Opm::PVTWRecord&            pvtw,
                               const Opm::UnitSystem&            usys,
                               SAaqArray&                        saaq)
            {
                using M = Opm::UnitSystem::measure;
                using Ix = VI::SAnalyticAquifer::index;

                auto cvrt = [&usys](const M unit, const double x) -> float
                {
                    return static_cast<float>(usys.from_si(unit, x));
                };

                // Unit hack: *to_si()* here since we don't have a compressibility unit.
                saaq[Ix::Compressibility] =
                    static_cast<float>(usys.to_si(M::pressure, aquifer.C_t));

                saaq[Ix::CTRadius] = cvrt(M::length, aquifer.r_o);
                saaq[Ix::CTPermeability] = cvrt(M::permeability, aquifer.k_a);
                saaq[Ix::CTPorosity] = cvrt(M::identity, aquifer.phi_aq);

                saaq[Ix::InitPressure] = cvrt(M::pressure, aquifer.p0.second);
                saaq[Ix::DatumDepth] = cvrt(M::length, aquifer.d0);

                saaq[Ix::CTThickness] = cvrt(M::length, aquifer.h);
                saaq[Ix::CTAngle] = cvrt(M::identity, aquifer.theta);

                const auto dp = aquifer.p0.second - pvtw.reference_pressure;
                const auto bw = pvtw.volume_factor * (1.0 - pvtw.compressibility*dp);
                saaq[Ix::CTWatMassDensity] = cvrt(M::density, rhoWS / bw);

                const auto mu = pvtw.viscosity * (1.0 + pvtw.viscosibility*dp);
                saaq[Ix::CTWatViscosity] = cvrt(M::viscosity, mu);
            }
        } // CarterTracy

        namespace Fetckovich {
            template <typename SAaqArray>
            void staticContrib(const Opm::Aquifetp::AQUFETP_data& aquifer,
                               const Opm::UnitSystem&             usys,
                               SAaqArray&                         saaq)
            {
                using M = Opm::UnitSystem::measure;
                using Ix = VI::SAnalyticAquifer::index;

                auto cvrt = [&usys](const M unit, const double x) -> float
                {
                    return static_cast<float>(usys.from_si(unit, x));
                };

                // Time constant
                const auto Tc = aquifer.C_t * aquifer.V0 / aquifer.J;

                // Unit hack: *to_si()* here since we don't have a compressibility unit.
                saaq[Ix::Compressibility] =
                    static_cast<float>(usys.to_si(M::pressure, aquifer.C_t));

                saaq[Ix::FetInitVol] = cvrt(M::liquid_surface_volume, aquifer.V0);
                saaq[Ix::FetProdIndex] = cvrt(M::liquid_productivity_index, aquifer.J);
                saaq[Ix::FetTimeConstant] = cvrt(M::time, Tc);

                saaq[Ix::InitPressure] = cvrt(M::pressure, aquifer.p0.second);
                saaq[Ix::DatumDepth] = cvrt(M::length, aquifer.d0);
            }
        } // Fetckovich
    } // SinglePrecAnalyticAquifer

    namespace SinglePrecAnalyticAquiferConn
    {
        std::vector<Opm::RestartIO::Helpers::WindowedArray<float>>
        allocate(const Opm::RestartIO::InteHEAD::AquiferDims& aqDims)
        {
            using WA = Opm::RestartIO::Helpers::WindowedArray<float>;

            return std::vector<WA>(aqDims.maxAquiferID, WA {
                WA::NumWindows{ static_cast<WA::Idx>(aqDims.maxNumActiveAquiferConn) },
                WA::WindowSize{ static_cast<WA::Idx>(aqDims.numRealConnElem) }
            });
        }

        template <typename SCAqArray>
        void staticContrib(const Opm::Aquancon::AquancCell& connection,
                           const double                     tot_influx,
                           SCAqArray&                       scaq)
        {
            using Ix = VI::SAnalyticAquiferConn::index;

            auto make_ratio = [tot_influx](const double x) -> float
            {
                return static_cast<float>(x / tot_influx);
            };

            scaq[Ix::InfluxFraction]        = make_ratio(connection.influx_coeff);
            scaq[Ix::FaceAreaToInfluxCoeff] = make_ratio(connection.effective_facearea);
        }
    } // SingleAnalyticAquiferConn

    namespace DoublePrecAnalyticAquifer
    {
        double totalInfluxCoefficient(const Opm::UnitSystem& usys,
                                      const double           tot_influx)
        {
            using M = Opm::UnitSystem::measure;
            return usys.from_si(M::length, usys.from_si(M::length, tot_influx));
        }

        Opm::RestartIO::Helpers::WindowedArray<double>
        allocate(const Opm::RestartIO::InteHEAD::AquiferDims& aqDims)
        {
            using WA = Opm::RestartIO::Helpers::WindowedArray<double>;

            return WA {
                WA::NumWindows{ static_cast<WA::Idx>(aqDims.maxAquiferID) },
                WA::WindowSize{ static_cast<WA::Idx>(aqDims.numDoubAquiferElem) }
            };
        }

        namespace Common {
            template <typename SummaryVariable, typename XAaqArray>
            void dynamicContrib(SummaryVariable&&      summaryVariable,
                                const double           tot_influx,
                                const Opm::UnitSystem& usys,
                                XAaqArray&             xaaq)
            {
                using Ix = VI::XAnalyticAquifer::index;

                xaaq[Ix::FlowRate]   = summaryVariable("AAQR");
                xaaq[Ix::Pressure]   = summaryVariable("AAQP");
                xaaq[Ix::ProdVolume] = summaryVariable("AAQT");

                xaaq[Ix::TotalInfluxCoeff] = totalInfluxCoefficient(usys, tot_influx);
            }
        } // Common

        namespace CarterTracy {
            template <typename SummaryVariable, typename XAaqArray>
            void dynamicContrib(SummaryVariable&&                 summaryVariable,
                                const Opm::AquiferCT::AQUCT_data& aquifer,
                                const Opm::PVTWRecord&            pvtw,
                                const double                      tot_influx,
                                const Opm::UnitSystem&            usys,
                                XAaqArray&                        xaaq)
            {
                using M = Opm::UnitSystem::measure;
                using Ix = VI::XAnalyticAquifer::index;

                Common::dynamicContrib(std::forward<SummaryVariable>(summaryVariable),
                                       tot_influx, usys, xaaq);

                const auto x = aquifer.phi_aq * aquifer.C_t * aquifer.r_o * aquifer.r_o;

                const auto dp   = aquifer.p0.second - pvtw.reference_pressure;
                const auto mu   = pvtw.viscosity * (1.0 + pvtw.viscosibility*dp);
                const auto Tc   = mu * x / (aquifer.c1 * aquifer.k_a);
                const auto beta = aquifer.c2 * aquifer.h * aquifer.theta * x;

                // Note: *to_si()* here since this is a *reciprocal* time constant
                xaaq[Ix::CTRecipTimeConst] = usys.to_si(M::time, 1.0 / Tc);

                // Note: *to_si()* for the pressure unit here since 'beta'
                // is total influx (volume) per unit pressure drop.
                xaaq[Ix::CTInfluxConstant] = usys.from_si(M::volume, usys.to_si(M::pressure, beta));

                xaaq[Ix::CTDimensionLessTime]     = summaryVariable("AAQTD");
                xaaq[Ix::CTDimensionLessPressure] = summaryVariable("AAQPD");
            }
        } // CarterTracy

        namespace Fetkovich {
            template <typename SummaryVariable, typename XAaqArray>
            void dynamicContrib(SummaryVariable&&      summaryVariable,
                                const double           tot_influx,
                                const Opm::UnitSystem& usys,
                                XAaqArray&             xaaq)
            {
                Common::dynamicContrib(std::forward<SummaryVariable>(summaryVariable),
                                       tot_influx, usys, xaaq);
            }
        } // Fetkovich
    } // DoublePrecAnalyticAquifer

    namespace DoublePrecNumericAquifer
    {
        Opm::RestartIO::Helpers::WindowedArray<double>
        allocate(const Opm::RestartIO::InteHEAD::AquiferDims& aqDims)
        {
            using WA = Opm::RestartIO::Helpers::WindowedArray<double>;

            return WA {
                WA::NumWindows{ static_cast<WA::Idx>(aqDims.numNumericAquiferRecords) },
                WA::WindowSize{ static_cast<WA::Idx>(aqDims.numNumericAquiferDoubleElem) }
            };
        }

        template <typename SummaryVariable, typename RAqnArray>
        void dynamicContrib(const Opm::NumericalAquiferCell& aqCell,
                            SummaryVariable&&                summaryVariable,
                            const Opm::UnitSystem&           usys,
                            RAqnArray&                       raqn)
        {
            using M = Opm::UnitSystem::measure;
            using Ix = VI::RNumericAquifer::index;

            raqn[Ix::Area] = usys.from_si(M::length, usys.from_si(M::length, aqCell.area));
            raqn[Ix::Length] = usys.from_si(M::length, aqCell.length);
            raqn[Ix::Porosity] = aqCell.porosity;
            raqn[Ix::Permeability] = usys.from_si(M::permeability, aqCell.permeability);
            raqn[Ix::Depth] = usys.from_si(M::length, aqCell.depth);

            if (aqCell.init_pressure.has_value()) {
                raqn[Ix::Pressure] = usys.from_si(M::pressure, aqCell.init_pressure.value());
            }

            raqn[Ix::Unknown_1] = 1.0; // Unknown item.  1.0 in all cases so far.
            raqn[Ix::Unknown_2] = 1.0; // Unknown item.  1.0 in all cases so far.
            raqn[Ix::Unknown_3] = 1.0; // Unknown item.  1.0 in all cases so far.

            raqn[Ix::PoreVolume] = usys.from_si(M::volume, aqCell.poreVolume());

            raqn[Ix::FlowRate]    = summaryVariable("ANQR");
            raqn[Ix::ProdVolume]  = summaryVariable("ANQT");
            raqn[Ix::DynPressure] = summaryVariable("ANQP");
        }
    } // IntegerNumericAquifer

    namespace DoublePrecAnalyticAquiferConn
    {
        std::vector<Opm::RestartIO::Helpers::WindowedArray<double>>
        allocate(const Opm::RestartIO::InteHEAD::AquiferDims& aqDims)
        {
            using WA = Opm::RestartIO::Helpers::WindowedArray<double>;

            return std::vector<WA>(aqDims.maxAquiferID, WA {
                WA::NumWindows{ static_cast<WA::Idx>(aqDims.maxNumActiveAquiferConn) },
                WA::WindowSize{ static_cast<WA::Idx>(aqDims.numDoubConnElem) }
            });
        }
    } // SingleAnalyticAquiferConn
} // Anonymous

Opm::RestartIO::Helpers::AggregateAquiferData::
AggregateAquiferData(const InteHEAD::AquiferDims& aqDims,
                     const AquiferConfig&         aqConfig,
                     const EclipseGrid&           grid)
    : maxActiveAnalyticAquiferID_   { aqDims.maxAquiferID }
    , numActiveConn_                ( aqDims.maxAquiferID, 0 )
    , totalInflux_                  ( aqDims.maxAquiferID, 0.0 )
    , integerAnalyticAq_            { IntegerAnalyticAquifer::       allocate(aqDims) }
    , singleprecAnalyticAq_         { SinglePrecAnalyticAquifer::    allocate(aqDims) }
    , doubleprecAnalyticAq_         { DoublePrecAnalyticAquifer::    allocate(aqDims) }
    , integerNumericAq_             { IntegerNumericAquifer::        allocate(aqDims) }
    , doubleprecNumericAq_          { DoublePrecNumericAquifer::     allocate(aqDims) }
    , integerAnalyticAquiferConn_   { IntegerAnalyticAquiferConn::   allocate(aqDims) }
    , singleprecAnalyticAquiferConn_{ SinglePrecAnalyticAquiferConn::allocate(aqDims) }
    , doubleprecAnalyticAquiferConn_{ DoublePrecAnalyticAquiferConn::allocate(aqDims) }
{
    if (! aqConfig.hasAnalyticalAquifer()) {
        return;
    }

    const auto map = buildColumnarActiveIndexMappingTables(grid);

    // Aquifer connections do not change in SCHEDULE.  Leverage that
    // property to compute static connection information exactly once.
    analyticAquiferConnectionLoop(aqConfig, [this, &grid, &map]
        (const std::size_t           aquiferID,
         const std::size_t           connectionID,
         const double                tot_influx,
         const Aquancon::AquancCell& connection) -> void
    {
        const auto aquIndex = static_cast<WindowedArray<int>::Idx>(aquiferID - 1);

        // Note: ACAQ intentionally omitted here.  This array is not fully characterised.
        auto icaq = this->integerAnalyticAquiferConn_   [aquIndex][connectionID];
        auto scaq = this->singleprecAnalyticAquiferConn_[aquIndex][connectionID];

        this->numActiveConn_[aquIndex] += 1;
        this->totalInflux_  [aquIndex]  = tot_influx;

        IntegerAnalyticAquiferConn::staticContrib(connection, grid, map, icaq);
        SinglePrecAnalyticAquiferConn::staticContrib(connection, tot_influx, scaq);
    });
}

void
Opm::RestartIO::Helpers::AggregateAquiferData::
captureDynamicdAquiferData(const AquiferConfig& aqConfig,
                           const SummaryState&  summaryState,
                           const PvtwTable&     pvtwTable,
                           const DensityTable&  densityTable,
                           const UnitSystem&    usys)
{
    FetkovichAquiferLoop(aqConfig, [this, &summaryState, &usys]
        (const Aquifetp::AQUFETP_data& aquifer)
    {
        const auto aquIndex = static_cast<WindowedArray<int>::Idx>(aquifer.aquiferID - 1);

        auto iaaq = this->integerAnalyticAq_[aquIndex];
        const auto nActiveConn = this->numActiveConn_[aquIndex];
        IntegerAnalyticAquifer::Fetckovich::staticContrib(aquifer, nActiveConn, iaaq);

        auto saaq = this->singleprecAnalyticAq_[aquIndex];
        SinglePrecAnalyticAquifer::Fetckovich::staticContrib(aquifer, usys, saaq);

        auto xaaq = this->doubleprecAnalyticAq_[aquIndex];
        const auto tot_influx = this->totalInflux_[aquIndex];
        auto sumVar = [&summaryState, &aquifer](const std::string& vector)
        {
            return getSummaryVariable(summaryState, vector, aquifer.aquiferID);
        };
        DoublePrecAnalyticAquifer::Fetkovich::dynamicContrib(sumVar, tot_influx, usys, xaaq);
    });

    CarterTracyAquiferLoop(aqConfig, [this, &summaryState, &pvtwTable, &densityTable, &usys]
        (const AquiferCT::AQUCT_data& aquifer)
    {
        const auto aquIndex = static_cast<WindowedArray<int>::Idx>(aquifer.aquiferID - 1);
        const auto pvtNum = static_cast<std::vector<double>::size_type>(aquifer.pvttableID - 1);

        auto iaaq = this->integerAnalyticAq_[aquIndex];
        const auto nActiveConn = this->numActiveConn_[aquIndex];
        IntegerAnalyticAquifer::CarterTracy::staticContrib(aquifer, nActiveConn, iaaq);

        auto saaq = this->singleprecAnalyticAq_[aquIndex];
        const auto rhoWS = densityTable[pvtNum].water;
        const auto& pvtw = pvtwTable[pvtNum];
        SinglePrecAnalyticAquifer::CarterTracy::staticContrib(aquifer, rhoWS,
                                                              pvtw, usys, saaq);

        auto xaaq = this->doubleprecAnalyticAq_[aquIndex];
        const auto tot_influx = this->totalInflux_[aquIndex];
        auto sumVar = [&summaryState, &aquifer](const std::string& vector)
        {
            return getSummaryVariable(summaryState, vector, aquifer.aquiferID);
        };
        DoublePrecAnalyticAquifer::CarterTracy::dynamicContrib(sumVar, aquifer, pvtw,
                                                               tot_influx, usys, xaaq);
    });

    numericAquiferLoop(aqConfig, [this, &summaryState, &usys]
        (const int aquiferID, const bool isNewID, const NumericalAquiferCell& aqCell)
    {
        auto iaqn = this->integerNumericAq_[aqCell.record_id];
        IntegerNumericAquifer::staticContrib(aqCell, aquiferID, iaqn);

        auto raqn = this->doubleprecNumericAq_[aqCell.record_id];
        auto sumVar = [&summaryState, aquiferID, isNewID](const std::string& vector)
        {
            return !isNewID ? 0.0 : getSummaryVariable(summaryState, vector, aquiferID);
        };
        DoublePrecNumericAquifer::dynamicContrib(aqCell, sumVar, usys, raqn);
    });
}
