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
                iaaq[Ix::TypeRelated1] = 1; // =1 for Carter-Tracy

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

                iaaq[Ix::Unknown_1] = 1; // Not characterised; =1 in all cases seen thus far.
            }
        } // Fetckovich
    } // IntegerAnalyticAquifer

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

        double getSummaryVariable(const Opm::SummaryState& summaryState,
                                  const std::string&       variable,
                                  const int                aquiferID)
        {
            const auto key = fmt::format("{}:{}", variable, aquiferID);

            if (summaryState.has(key)) {
                return summaryState.get(key);
            }

            return 0.0;
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
    , integerAnalyticAquiferConn_   { IntegerAnalyticAquiferConn::   allocate(aqDims) }
    , singleprecAnalyticAquiferConn_{ SinglePrecAnalyticAquiferConn::allocate(aqDims) }
    , doubleprecAnalyticAquiferConn_{ DoublePrecAnalyticAquiferConn::allocate(aqDims) }
{
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
            return DoublePrecAnalyticAquifer::
                getSummaryVariable(summaryState, vector, aquifer.aquiferID);
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
            return DoublePrecAnalyticAquifer::
                getSummaryVariable(summaryState, vector, aquifer.aquiferID);
        };
        DoublePrecAnalyticAquifer::CarterTracy::dynamicContrib(sumVar, aquifer, pvtw,
                                                               tot_influx, usys, xaaq);
    });
}
