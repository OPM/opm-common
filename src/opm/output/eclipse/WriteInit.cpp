/*
  Copyright (c) 2019 Equinor ASA
  Copyright (c) 2016 Statoil ASA
  Copyright (c) 2013-2015 Andreas Lauser
  Copyright (c) 2013 SINTEF ICT, Applied Mathematics.
  Copyright (c) 2013 Uni Research AS
  Copyright (c) 2015 IRIS AS

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

#include <opm/output/eclipse/WriteInit.hpp>

#include <opm/io/eclipse/OutputStream.hpp>

#include <opm/output/data/Solution.hpp>
#include <opm/output/eclipse/Tables.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/NNC.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <cstddef>
#include <exception>
#include <stdexcept>
#include <utility>

namespace {

    struct CellProperty
    {
        std::string                name;
        ::Opm::UnitSystem::measure unit;
    };

    using Properties = std::vector<CellProperty>;

    std::vector<float> singlePrecision(const std::vector<double>& x)
    {
        return { x.begin(), x.end() };
    }

    void writeInitFileHeader(const ::Opm::EclipseState&      es,
                             const ::Opm::EclipseGrid&       grid,
                             const ::Opm::Schedule&          sched,
                             Opm::EclIO::OutputStream::Init& initFile)
    {
        {
            const auto ih = ::Opm::RestartIO::Helpers::
                createInteHead(es, grid, sched, 0.0, 0, 0);

            initFile.write("INTEHEAD", ih);
        }

        {
            const auto lh = ::Opm::RestartIO::Helpers::
                createLogiHead(es);

            initFile.write("LOGIHEAD", lh);
        }

        {
            const auto dh = ::Opm::RestartIO::Helpers::
                createDoubHead(es, sched, 0, 0.0, 0.0);

            initFile.write("DOUBHEAD", dh);
        }
    }

    void writePoreVolume(const ::Opm::EclipseState&        es,
                         const ::Opm::EclipseGrid&         grid,
                         const ::Opm::UnitSystem&          units,
                         ::Opm::EclIO::OutputStream::Init& initFile)
    {
        auto porv = es.get3DProperties()
            .getDoubleGridProperty("PORV").getData();

        for (auto nGlob    = porv.size(),
                  globCell = 0*nGlob; globCell < nGlob; ++globCell)
        {
            if (! grid.cellActive(globCell)) {
                porv[globCell] = 0.0;
            }
        }

        units.from_si(::Opm::UnitSystem::measure::volume, porv);

        initFile.write("PORV", singlePrecision(porv));
    }

    void writeGridGeometry(const ::Opm::EclipseGrid&         grid,
                           const ::Opm::UnitSystem&          units,
                           ::Opm::EclIO::OutputStream::Init& initFile)
    {
        const auto length = ::Opm::UnitSystem::measure::length;
        const auto nAct   = grid.getNumActive();

        auto dx    = std::vector<float>{};  dx   .reserve(nAct);
        auto dy    = std::vector<float>{};  dy   .reserve(nAct);
        auto dz    = std::vector<float>{};  dz   .reserve(nAct);
        auto depth = std::vector<float>{};  depth.reserve(nAct);

        for (auto cell = 0*nAct; cell < nAct; ++cell) {
            const auto  globCell = grid.getGlobalIndex(cell);
            const auto& dims     = grid.getCellDims(globCell);

            dx   .push_back(units.from_si(length, dims[0]));
            dy   .push_back(units.from_si(length, dims[1]));
            dz   .push_back(units.from_si(length, dims[2]));
            depth.push_back(units.from_si(length, grid.getCellDepth(globCell)));
        }

        initFile.write("DEPTH", depth);
        initFile.write("DX"   , dx);
        initFile.write("DY"   , dy);
        initFile.write("DZ"   , dz);
    }

    template <typename T, class WriteVector>
    void writeCellProperties(const Properties&               propList,
                             const ::Opm::GridProperties<T>& propValues,
                             const ::Opm::EclipseGrid&       grid,
                             WriteVector&&                   write)
    {
        for (const auto& prop : propList) {
            if (! propValues.hasKeyword(prop.name)) {
                continue;
            }

            const auto& opm_property = propValues.getKeyword(prop.name);

            write(prop, opm_property.compressedCopy(grid));
        }
    }

    void writeDoubleCellProperties(const Properties&                    propList,
                                   const ::Opm::GridProperties<double>& propValues,
                                   const ::Opm::EclipseGrid&            grid,
                                   const ::Opm::UnitSystem&             units,
                                   ::Opm::EclIO::OutputStream::Init&    initFile)
    {
        writeCellProperties(propList, propValues, grid,
            [&units, &initFile](const CellProperty&   prop,
                                std::vector<double>&& value)
        {
            units.from_si(prop.unit, value);
            initFile.write(prop.name, singlePrecision(value));
        });
    }

    void writeDoubleCellProperties(const ::Opm::EclipseState&        es,
                                   const ::Opm::EclipseGrid&         grid,
                                   const ::Opm::UnitSystem&          units,
                                   ::Opm::EclIO::OutputStream::Init& initFile)
    {
        const auto doubleKeywords = Properties {
            {"PORO"  , ::Opm::UnitSystem::measure::identity },
            {"PERMX" , ::Opm::UnitSystem::measure::permeability },
            {"PERMY" , ::Opm::UnitSystem::measure::permeability },
            {"PERMZ" , ::Opm::UnitSystem::measure::permeability },
            {"NTG"   , ::Opm::UnitSystem::measure::identity },
        };

        // The INIT file should always contain the NTG property, we
        // therefore invoke the auto create functionality to ensure
        // that "NTG" is included in the properties container.
        const auto& properties = es.get3DProperties().getDoubleProperties();
        properties.assertKeyword("NTG");

        writeDoubleCellProperties(doubleKeywords, properties,
                                  grid, units, initFile);
    }

    void writeIntegerCellProperties(const ::Opm::EclipseState&        es,
                                    const ::Opm::EclipseGrid&         grid,
                                    ::Opm::EclIO::OutputStream::Init& initFile)
    {
        const auto& properties = es.get3DProperties().getIntProperties();

        // The INIT file should always contain PVT, saturation function,
        // equilibration, and fluid-in-place region vectors.  Call
        // assertKeyword() here--on a 'const' GridProperties object--to
        // invoke the autocreation property, and ensure that the keywords
        // exist in the properties container.
        properties.assertKeyword("PVTNUM");
        properties.assertKeyword("SATNUM");
        properties.assertKeyword("EQLNUM");
        properties.assertKeyword("FIPNUM");

        for (const auto& property : properties) {
            auto ecl_data = property.compressedCopy(grid);

            initFile.write(property.getKeywordName(), ecl_data);
        }
    }

    void writeSimulatorProperties(const ::Opm::EclipseGrid&         grid,
                                  const ::Opm::data::Solution&      simProps,
                                  ::Opm::EclIO::OutputStream::Init& initFile)
    {
        for (const auto& prop : simProps) {
            const auto& value = grid.compressedVector(prop.second.data);

            initFile.write(prop.first, singlePrecision(value));
        }
    }

    void writeTableData(const ::Opm::EclipseState&        es,
                        const ::Opm::UnitSystem&          units,
                        ::Opm::EclIO::OutputStream::Init& initFile)
    {
        ::Opm::Tables tables(units);

        tables.addPVTTables(es);
        tables.addDensity(es.getTableManager().getDensityTable());
        tables.addSatFunc(es);

        initFile.write("TABDIMS", tables.tabdims());
        initFile.write("TAB"    , tables.tab());
    }

    void writeIntegerMaps(std::map<std::string, std::vector<int>> mapData,
                          ::Opm::EclIO::OutputStream::Init&       initFile)
    {
        for (const auto& pair : mapData) {
            const auto& key = pair.first;

            if (key.size() > std::size_t{8}) {
                throw std::invalid_argument {
                    "Keyword '" + key + "'is too long."
                };
            }

            initFile.write(key, pair.second);
        }
    }

    void writeNonNeighbourConnections(const ::Opm::NNC&                 nnc,
                                      const ::Opm::UnitSystem&          units,
                                      ::Opm::EclIO::OutputStream::Init& initFile)
    {
        auto tran = std::vector<double>{};
        tran.reserve(nnc.numNNC());

        for (const auto& nd : nnc.nncdata()) {
            tran.push_back(nd.trans);
        }

        units.from_si(::Opm::UnitSystem::measure::transmissibility, tran);

        initFile.write("TRANNNC", singlePrecision(tran));
    }
} // Anonymous namespace

void Opm::InitIO::write(const ::Opm::EclipseState&              es,
                        const ::Opm::EclipseGrid&               grid,
                        const ::Opm::Schedule&                  schedule,
                        const ::Opm::data::Solution&            simProps,
                        std::map<std::string, std::vector<int>> int_data,
                        const ::Opm::NNC&                       nnc,
                        ::Opm::EclIO::OutputStream::Init&       initFile)
{
    const auto& units = es.getUnits();

    writeInitFileHeader(es, grid, schedule, initFile);

    // The PORV vector is a special case.  This particular vector always
    // holds a total of nx*ny*nz elements, and the elements are explicitly
    // set to zero for inactive cells.  This treatment implies that the
    // active/inactive cell mapping can be inferred by reading the PORV
    // vector from the result set.
    writePoreVolume(es, grid, units, initFile);

    writeGridGeometry(grid, units, initFile);

    writeDoubleCellProperties(es, grid, units, initFile);

    writeSimulatorProperties(grid, simProps, initFile);

    writeTableData(es, units, initFile);

    writeIntegerCellProperties(es, grid, initFile);

    writeIntegerMaps(std::move(int_data), initFile);

    if (true || (nnc.numNNC() > std::size_t{0})) {
        writeNonNeighbourConnections(nnc, units, initFile);
    }
}
