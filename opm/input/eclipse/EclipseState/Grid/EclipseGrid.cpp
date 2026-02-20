/*
  Copyright 2014 Statoil ASA.

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

#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <opm/common/ErrorMacros.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/utility/String.hpp>
#include <opm/common/utility/numeric/GeometryUtil.hpp>
#include <opm/common/utility/numeric/VectorOps.hpp>
#include <opm/common/utility/numeric/VectorUtil.hpp>
#include <opm/common/utility/numeric/calculateCellVol.hpp>

#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/EclOutput.hpp>
#include <opm/io/eclipse/PaddedOutputString.hpp>

#include <opm/input/eclipse/EclipseState/Grid/LgrCollection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldProps.hpp>
#include <opm/input/eclipse/EclipseState/Grid/NNC.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>
#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/input/eclipse/Deck/DeckSection.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/A.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/D.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/I.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/M.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/R.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/S.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/T.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/Z.hpp>

#define _USE_MATH_DEFINES

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include <fmt/format.h>

namespace Opm {

namespace {


std::optional<UnitSystem> make_grid_units(const std::string& grid_unit) {
    if (grid_unit == "METRES")
        return UnitSystem(UnitSystem::UnitType::UNIT_TYPE_METRIC);

    if (grid_unit == "FEET")
        return UnitSystem(UnitSystem::UnitType::UNIT_TYPE_FIELD);

    if (grid_unit == "CM")
        return UnitSystem(UnitSystem::UnitType::UNIT_TYPE_LAB);

    return std::nullopt;
}

void apply_GRIDUNIT(const UnitSystem& deck_units, const UnitSystem& grid_units, std::vector<double>& data)
{
    double scale_factor = grid_units.getDimension(UnitSystem::measure::length).getSIScaling() / deck_units.getDimension(UnitSystem::measure::length).getSIScaling();
    std::ranges::transform(data, data.begin(),
                           [scale_factor](const auto& v) { return v * scale_factor; });
}

}
EclipseGrid::EclipseGrid()
    : GridDims(),
      m_minpvMode(MinpvMode::Inactive),
      m_pinchoutMode(PinchMode::TOPBOT),
      m_multzMode(PinchMode::TOP),
      m_pinchGapMode(PinchMode::GAP),
      m_pinchMaxEmptyGap(ParserKeywords::PINCH::MAX_EMPTY_GAP::defaultValue)
{}

EclipseGrid::EclipseGrid(const std::array<int, 3>& dims ,
                         const std::vector<double>& coord ,
                         const std::vector<double>& zcorn ,
                         const int * actnum)
    : GridDims(dims),
      m_minpvMode(MinpvMode::Inactive),
      m_pinchoutMode(PinchMode::TOPBOT),
      m_multzMode(PinchMode::TOP),
      m_pinchGapMode(PinchMode::GAP),
      m_pinchMaxEmptyGap(ParserKeywords::PINCH::MAX_EMPTY_GAP::defaultValue)
{
    initCornerPointGrid( coord , zcorn , actnum );
}

/**
   Will create an EclipseGrid instance based on an existing
   GRID/EGRID file.
*/


EclipseGrid::EclipseGrid(const std::string& fileName )
    : GridDims(),
      m_minpvMode(MinpvMode::Inactive),
      m_pinchoutMode(PinchMode::TOPBOT),
      m_multzMode(PinchMode::TOP),
      m_pinchGapMode(PinchMode::GAP),
      m_pinchMaxEmptyGap(ParserKeywords::PINCH::MAX_EMPTY_GAP::defaultValue)
{

    Opm::EclIO::EclFile egridfile(fileName);
    m_useActnumFromGdfile=true;
    initGridFromEGridFile(egridfile, fileName);
}


EclipseGrid::EclipseGrid(const GridDims& gd)
    : GridDims(gd),
      m_minpvMode(MinpvMode::Inactive),
      m_pinchoutMode(PinchMode::TOPBOT),
      m_multzMode(PinchMode::TOP),
      m_pinchGapMode(PinchMode::GAP),
      m_pinchMaxEmptyGap(ParserKeywords::PINCH::MAX_EMPTY_GAP::defaultValue)
{
    this->m_nactive = this->getCartesianSize();
    this->active_volume = std::nullopt;
    // Nothing else initialized. Leaving in particular as empty:
    // m_actnum,
    // m_global_to_active,
    // m_active_to_global.
    //
    // EclipseGrid will only be usable for constructing Box objects with
    // all cells active.
}


EclipseGrid::EclipseGrid(std::size_t nx, std::size_t ny , std::size_t nz,
                         double dx, double dy, double dz, double top)
    : GridDims(nx, ny, nz),
      m_minpvMode(MinpvMode::Inactive),
      m_pinchoutMode(PinchMode::TOPBOT),
      m_multzMode(PinchMode::TOP),
      m_pinchGapMode(PinchMode::GAP),
      m_pinchMaxEmptyGap(ParserKeywords::PINCH::MAX_EMPTY_GAP::defaultValue)
{

    m_coord.reserve((nx+1)*(ny+1)*6);

    for (std::size_t j = 0; j < (ny+1); j++) {
        for (std::size_t i = 0; i < (nx+1); i++) {
            m_coord.push_back(i*dx);
            m_coord.push_back(j*dy);
            m_coord.push_back(0.0);
            m_coord.push_back(i*dx);
            m_coord.push_back(j*dy);
            m_coord.push_back(nz*dz);
        }
    }

    m_zcorn.assign(nx*ny*nz*8, 0);

    for (std::size_t k = 0; k < nz ; k++) {
        for (std::size_t j = 0; j < ny ; j++) {
            for (std::size_t i = 0; i < nx ; i++) {

                // top face of cell
                int zind = i*2 + j*nx*4 + k*nx*ny*8;

                double zt = top + k*dz;
                double zb = top + (k+1)*dz;

                m_zcorn[zind] = zt;
                m_zcorn[zind + 1] = zt;

                zind = zind + nx*2;

                m_zcorn[zind] = zt;
                m_zcorn[zind + 1] = zt;

                // bottom face of cell
                zind = i*2 + j*nx*4 + k*nx*ny*8 + nx*ny*4;

                m_zcorn[zind] = zb;
                m_zcorn[zind + 1] = zb;

                zind = zind + nx*2;

                m_zcorn[zind] = zb;
                m_zcorn[zind + 1] = zb;
            }
        }
    }

    resetACTNUM();
}

EclipseGrid::EclipseGrid(const EclipseGrid& src, const double* zcorn, const std::vector<int>& actnum)
    : EclipseGrid(src)
{

    if (zcorn != nullptr) {
        std::size_t sizeZcorn = this->getCartesianSize()*8;

        for (std::size_t n=0; n < sizeZcorn; n++) {
            m_zcorn[n] = zcorn[n];
        }

        ZcornMapper mapper( getNX(), getNY(), getNZ());
        zcorn_fixed = mapper.fixupZCORN( m_zcorn );
    }

    resetACTNUM(actnum);
}

EclipseGrid::EclipseGrid(const EclipseGrid& src, const std::vector<int>& actnum)
        : EclipseGrid( src , nullptr , actnum )
{ }

/*
  This is the main EclipseGrid constructor, it will inspect the
  input Deck for grid keywords, either the corner point keywords
  COORD and ZCORN, or the various rectangular keywords like DX,DY
  and DZ.

  Actnum is treated specially:

    1. If an actnum pointer is passed in that should be a pointer
       to 0 and 1 values which will be used as actnum mask.

    2. If the actnum pointer is not present the constructor will
       look in the deck for an actnum keyword, and use that if it
       is found. This is a best effort which will work in many
       cases, but if the ACTNUM keyword is manipulated in the deck
       those manipulations will be silently lost; if the ACTNUM
       keyword has size different from nx*ny*nz it will also be
       silently ignored.

  With a mutable EclipseGrid instance you can later call the
  EclipseGrid::resetACTNUM() method when you have complete actnum
  information. The EclipseState based construction of EclipseGrid
  is a two-pass operation, which guarantees that actnum is handled
  correctly.
*/



EclipseGrid::EclipseGrid(const Deck& deck, const int * actnum)
    : GridDims(deck),
      m_minpvMode(MinpvMode::Inactive),
      m_pinchoutMode(PinchMode::TOPBOT),
      m_multzMode(PinchMode::TOP),
      m_pinchGapMode(PinchMode::GAP),
      m_pinchMaxEmptyGap(ParserKeywords::PINCH::MAX_EMPTY_GAP::defaultValue)
{
    if (deck.hasKeyword("GDFILE")){

        if (deck.hasKeyword("ACTNUM")){
            if (keywInputBeforeGdfile(deck, "ACTNUM"))  {
                m_useActnumFromGdfile = true;
            }
        } else {
            m_useActnumFromGdfile = true;
        }

    }

    updateNumericalAquiferCells(deck);

    initGrid(deck, actnum);

    if (deck.hasKeyword<ParserKeywords::MAPAXES>())
        this->m_mapaxes = std::make_optional<MapAxes>( deck );

    /*
      The GRIDUNIT handling is simplified compared to the full specification:

        1. The optional second item 'MAP' is ignored.

        2. In Eclipse the action of the GRIDUNIT keyword only applies to
           keywords in the same file as the GRIDUNIT keyword itself is in; in
           our implementation we apply the GRIDUNIT transformation if the deck
           contains the GRIDUNUT keyword - irrespective of exactly where it is
           located in the deck.
    */

    if (deck.hasKeyword<ParserKeywords::GRIDUNIT>()) {
        const auto& kw = deck.get<ParserKeywords::GRIDUNIT>().front();
        const auto& length_unit = trim_copy(kw.getRecord(0).getItem(0).get<std::string>(0));
        auto grid_units = make_grid_units(length_unit);
        if (!grid_units.has_value())
            throw OpmInputError(fmt::format("Invalid length specifier: [{}]", length_unit), kw.location());

        if (grid_units.value() != deck.getActiveUnitSystem()) {
            apply_GRIDUNIT(deck.getActiveUnitSystem(), grid_units.value(), this->m_zcorn);
            apply_GRIDUNIT(deck.getActiveUnitSystem(), grid_units.value(), this->m_coord);
            if (this->m_rv.has_value())
                apply_GRIDUNIT(deck.getActiveUnitSystem(), grid_units.value(), this->m_rv.value());

            if (this->m_input_coord.has_value()) {
                apply_GRIDUNIT(deck.getActiveUnitSystem(), grid_units.value(), this->m_input_coord.value());
            }

            if (this->m_input_zcorn.has_value()) {
                apply_GRIDUNIT(deck.getActiveUnitSystem(), grid_units.value(), this->m_input_zcorn.value());
            }
        }
    }
}


    bool EclipseGrid::circle( ) const{
        return this->m_circle;
    }


    void EclipseGrid::initGrid(const Deck& deck, const int* actnum)
    {
        enum GridType { COORD, DEPTHZ, TOPS, RADIAL, SPIDER, GDFILE, GT_SIZE };

        std::vector<int> found;
        if (hasCornerPointKeywords(deck)) {
            found.push_back(GridType::COORD);
        }
        if (hasDVDEPTHZKeywords(deck)) {
            found.push_back(GridType::DEPTHZ);
        }
        if (hasDTOPSKeywords(deck)) {
            found.push_back(GridType::TOPS);
        }
        if (hasRadialKeywords(deck)) {
            found.push_back(GridType::RADIAL);
        }
        if (hasSpiderKeywords(deck)) {
            found.push_back(GridType::SPIDER);
        }
        if (hasGDFILE(deck)) {
            found.push_back(GridType::GDFILE);
        }

        const std::string messages[GT_SIZE] {
            "COORD with ZCORN creates a corner-point grid",
            "DEPTHZ with DXV, DYV, DZV creates a cartesian grid",
            "TOPS with DX/DXV, DY/DYV, DZ/DZV creates a cartesian grid",
            "RADIAL with DR/DRV, DTHETA/DTHETAV, DZ/DZV and TOPS creates a cylindrical grid",
            "SPIDER with DR/DRV, DTHETA/DTHETAV, DZ/DZV and TOPS creates a spider grid",
            "GDFILE reads a grid from file"};

        if (found.empty()) {
            std::string message = "The grid must be specified using one of these options:";
            for (int grid_type = 0; grid_type < GridType::GT_SIZE; grid_type++) {
                message += "\n    " + messages[grid_type];
            }
            throw std::invalid_argument(message);
        }

        if (found.size() > 1) {
            std::string message = "The specification of the grid is ambiguous:";
            for (const int& grid_type : found) {
                message += "\n    " + messages[grid_type];
            }
            throw std::invalid_argument(message);
        }

        switch (found.front()) {
        case GridType::COORD:
            this->initCornerPointGrid(deck);
            break;
        case GridType::DEPTHZ:
        case GridType::TOPS:
            this->initCartesianGrid(deck);
            break;
        case GridType::RADIAL:
            this->initCylindricalGrid(deck);
            break;
        case GridType::SPIDER:
            this->initSpiderwebGrid(deck);
            break;
        case GridType::GDFILE:
            this->initBinaryGrid(deck);
            break;
        }

        if (deck.hasKeyword<ParserKeywords::PINCH>()) {
            const auto& record = deck.get<ParserKeywords::PINCH>( ).back().getRecord(0);
            const auto& item = record.getItem<ParserKeywords::PINCH::THRESHOLD_THICKNESS>( );
            m_pinch = item.getSIDouble(0);

            auto pinchoutString = record.getItem<ParserKeywords::PINCH::PINCHOUT_OPTION>().get< std::string >(0);
            m_pinchoutMode = PinchModeFromString(pinchoutString);

            auto multzString = record.getItem<ParserKeywords::PINCH::MULTZ_OPTION>().get< std::string >(0);
            m_multzMode = PinchModeFromString(multzString);
            auto pinchGapString = record.getItem<ParserKeywords::PINCH::CONTROL_OPTION>().get< std::string >(0);
            m_pinchGapMode = PinchModeFromString(pinchGapString);
            m_pinchMaxEmptyGap = record.getItem<ParserKeywords::PINCH::MAX_EMPTY_GAP>().getSIDouble(0);
        }

        m_minpvVector.resize(getCartesianSize(), 0.0);
        if (deck.hasKeyword<ParserKeywords::MINPV>()) {
            const auto& record = deck.get<ParserKeywords::MINPV>( ).back().getRecord(0);
            const auto& item = record.getItem<ParserKeywords::MINPV::VALUE>( );
            std::fill(m_minpvVector.begin(), m_minpvVector.end(), item.getSIDouble(0));
            m_minpvMode = MinpvMode::EclSTD;
        } else if (deck.hasKeyword<ParserKeywords::MINPORV>()) {
            const auto& record = deck.get<ParserKeywords::MINPORV>( ).back().getRecord(0);
            const auto& item = record.getItem<ParserKeywords::MINPORV::VALUE>( );
            std::fill(m_minpvVector.begin(), m_minpvVector.end(), item.getSIDouble(0));
            m_minpvMode = MinpvMode::EclSTD;
        }
        // Note that MINPVV is not handled here but in a second stage in EclipseState where we
        // call setMINPVV to also support BOX via grid properties

        if (actnum != nullptr) {
            this->resetACTNUM(actnum);
        }
        else if (! this->m_useActnumFromGdfile) {
            const auto fp = FieldProps {
                deck, EclipseGrid { static_cast<GridDims&>(*this) }
            };

            this->resetACTNUM(fp.actnumRaw());
        }
    }

    void EclipseGrid::initGridFromEGridFile(Opm::EclIO::EclFile& egridfile,
                                            const std::string&   fileName)
    {
        OpmLog::info(fmt::format("\nCreating grid from: {} ", fileName));

        if (!egridfile.hasKey("GRIDHEAD")) {
            throw std::invalid_argument("file: " + fileName + " is not a valid egrid file, GRIDHEAD not found");
        }

        if (!egridfile.hasKey("COORD")) {
            throw std::invalid_argument("file: " + fileName + " is not a valid egrid file, COORD not found");
        }

        if (!egridfile.hasKey("ZCORN")) {
            throw std::invalid_argument("file: " + fileName + " is not a valid egrid file, ZCORN not found");
        }

        if (!egridfile.hasKey("GRIDUNIT")) {
            throw std::invalid_argument("file: " + fileName + " is not a valid egrid file, GRIDUNIT not found");
        }

        {
            const std::vector<int>& gridhead = egridfile.get<int>("GRIDHEAD");

            this->m_nx = gridhead[1];
            this->m_ny = gridhead[2];
            this->m_nz = gridhead[3];
        }

        {
            const std::vector<float>& coord_f = egridfile.get<float>("COORD");
            const std::vector<float>& zcorn_f = egridfile.get<float>("ZCORN");

            m_coord.assign(coord_f.begin(), coord_f.end());
            m_zcorn.assign(zcorn_f.begin(), zcorn_f.end());
        }

        if (const auto& gridunit = egridfile.get<std::string>("GRIDUNIT");
            gridunit[0] != "METRES")
        {
            constexpr auto length = ::Opm::UnitSystem::measure::length;

            if (gridunit[0] == "FEET") {
                Opm::UnitSystem units(Opm::UnitSystem::UnitType::UNIT_TYPE_FIELD);
                units.to_si(length, m_coord);
                units.to_si(length, m_zcorn);
            }
            else if (gridunit[0] == "CM") {
                Opm::UnitSystem units(Opm::UnitSystem::UnitType::UNIT_TYPE_LAB);
                units.to_si(length, m_coord);
                units.to_si(length, m_zcorn);
            }
            else {
                std::string message = "gridunit '" + gridunit[0] + "' doesn't correspong to a valid unit system";
                throw std::invalid_argument(message);
            }
        }

        if (egridfile.hasKey("ACTNUM") && m_useActnumFromGdfile) {
            resetACTNUM(egridfile.get<int>("ACTNUM"));
        }
        else {
            this->resetACTNUM();
        }

        if (egridfile.hasKey("MAPAXES")) {
            this->m_mapaxes = std::make_optional<MapAxes>(egridfile);
        }

        ZcornMapper mapper(getNX(), getNY(), getNZ());
        zcorn_fixed = mapper.fixupZCORN(m_zcorn);
    }

    bool EclipseGrid::keywInputBeforeGdfile(const Deck& deck, const std::string& keyword) const {

        std::vector<std::string> keywordList;
        keywordList.reserve(deck.size());

        for (std::size_t n=0;n<deck.size();n++){
            DeckKeyword kw = deck[n];
            keywordList.push_back(kw.name());
        }

        int indKeyw = -1;
        int indGdfile = -1;

        for (std::size_t i = 0; i < keywordList.size(); i++){
            if (keywordList[i]=="GDFILE"){
                indGdfile=i;
            }

            if (keywordList[i]==keyword){
                indKeyw=i;
            }
        }

        if (indGdfile == -1) {
            throw std::runtime_error("keyword GDFILE not found in deck ");
        }

        if (indKeyw == -1) {
            throw std::runtime_error("keyword " + keyword + " not found in deck ");
        }

        return indKeyw < indGdfile;
    }


    std::size_t EclipseGrid::activeIndex(std::size_t i, std::size_t j, std::size_t k) const {

        return activeIndex( getGlobalIndex( i,j,k ));
    }

    std::size_t EclipseGrid::activeIndex(std::size_t globalIndex) const {
        if (m_global_to_active.empty()) {
            return globalIndex;
        }

        if (m_global_to_active[ globalIndex] == -1) {
            throw std::invalid_argument("Input argument does not correspond to an active cell");
        }

        return m_global_to_active[ globalIndex];
    }

    /**
       Observe: the input argument is assumed to be in the space
       [0,num_active).
    */

    std::size_t EclipseGrid::getGlobalIndex(std::size_t active_index) const {
        return m_active_to_global.at(active_index);
    }

    bool EclipseGrid::isPinchActive( ) const {
        return m_pinch.has_value();
    }

    double EclipseGrid::getPinchThresholdThickness( ) const {
        return m_pinch.value();
    }

    PinchMode EclipseGrid::getPinchOption( ) const {
        return m_pinchoutMode;
    }

    PinchMode EclipseGrid::getMultzOption( ) const {
        return m_multzMode;
    }

    MinpvMode EclipseGrid::getMinpvMode() const {
        return m_minpvMode;
    }

    PinchMode EclipseGrid::getPinchGapMode() const {
        return m_pinchGapMode;
    }

    double EclipseGrid::getPinchMaxEmptyGap() const {
        return m_pinchMaxEmptyGap;
    }


    const std::vector<double>& EclipseGrid::getMinpvVector( ) const {
        return m_minpvVector;
    }

    void EclipseGrid::initBinaryGrid(const Deck& deck)
    {
        const auto& gdfile = deck["GDFILE"].back().getRecord(0);

        const auto formatted = EclIO::EclFile::Formatted {
            gdfile.getItem("formatted").getTrimmedString(0).front() == 'F'
        };

        auto filename = deck
            .makeDeckPath(gdfile.getItem("filename").getTrimmedString(0));

        std::unique_ptr<EclIO::EclFile> egridfile{};

        // Some windows applications export .DATA files with relative GDFILE
        // keywords with a windows formatted path.  If open fails we give it
        // one more try with the replacement '\'' -> '/'.
        try {
            egridfile = std::make_unique<Opm::EclIO::EclFile>(filename, formatted);
        }
        catch (const std::runtime_error&) {
            std::replace(filename.begin(), filename.end(), '\\', '/');
            egridfile = std::make_unique<Opm::EclIO::EclFile>(filename, formatted);
        }

        this->initGridFromEGridFile(*egridfile, filename);
    }

    void EclipseGrid::initCartesianGrid(const Deck& deck) {

        if (hasDVDEPTHZKeywords( deck )) {
            initDVDEPTHZGrid(deck );
        } else if (hasDTOPSKeywords(deck)) {
            initDTOPSGrid( deck );
        } else {
            throw std::invalid_argument("Tried to initialize cartesian grid without all required keywords");
        }
    }

    void EclipseGrid::initDVDEPTHZGrid(const Deck& deck) {
      OpmLog::info(fmt::format("\nCreating grid from keywords DXV, DYV, DZV and DEPTHZ"));
        const std::vector<double>& DXV = deck.get<ParserKeywords::DXV>().back().getSIDoubleData();
        const std::vector<double>& DYV = deck.get<ParserKeywords::DYV>().back().getSIDoubleData();
        const std::vector<double>& DZV = deck.get<ParserKeywords::DZV>().back().getSIDoubleData();
        const std::vector<double>& DEPTHZ = deck.get<ParserKeywords::DEPTHZ>().back().getSIDoubleData();
        auto nx = this->getNX();
        auto ny = this->getNY();
        auto nz = this->getNZ();

        assertVectorSize( DEPTHZ , static_cast<size_t>( (nx + 1)*(ny +1 )) , "DEPTHZ");
        assertVectorSize( DXV    , static_cast<size_t>( nx ) , "DXV");
        assertVectorSize( DYV    , static_cast<size_t>( ny ) , "DYV");
        assertVectorSize( DZV    , static_cast<size_t>( nz ) , "DZV");

        m_coord = makeCoordDxvDyvDzvDepthz(DXV, DYV, DZV, DEPTHZ);
        m_zcorn = makeZcornDzvDepthz(DZV, DEPTHZ);

        ZcornMapper mapper( getNX(), getNY(), getNZ());
        zcorn_fixed = mapper.fixupZCORN( m_zcorn );
    }

    void EclipseGrid::initDTOPSGrid(const Deck& deck) {
      OpmLog::info(fmt::format("\nCreating grid from keywords DX, DY, DZ and TOPS"));

        std::vector<double> DX = EclipseGrid::createDVector(  this->getNXYZ(), 0 , "DX" , "DXV" , deck);
        std::vector<double> DY = EclipseGrid::createDVector(  this->getNXYZ(), 1 , "DY" , "DYV" , deck);
        std::vector<double> DZ = EclipseGrid::createDVector(  this->getNXYZ(), 2 , "DZ" , "DZV" , deck);
        std::vector<double> TOPS = EclipseGrid::createTOPSVector( this->getNXYZ(), DZ , deck );

        m_coord = makeCoordDxDyDzTops(DX, DY, DZ, TOPS);
        m_zcorn = makeZcornDzTops(DZ, TOPS);

        ZcornMapper mapper( getNX(), getNY(), getNZ());
        zcorn_fixed = mapper.fixupZCORN( m_zcorn );
    }


    void EclipseGrid::getCellCorners(const std::array<int, 3>& ijk, const std::array<int, 3>& dims,
                                     std::array<double,8>& X,
                                     std::array<double,8>& Y,
                                     std::array<double,8>& Z) const
    {

        std::array<int, 8> zind;
        std::array<int, 4> pind;

        // calculate indices for grid pillars in COORD arrray
        const std::size_t p_offset = ijk[1]*(dims[0]+1)*6 + ijk[0]*6;

        pind[0] = p_offset;
        pind[1] = p_offset + 6;
        pind[2] = p_offset + (dims[0]+1)*6;
        pind[3] = pind[2] + 6;

        // get depths from zcorn array in ZCORN array
        const std::size_t z_offset = ijk[2]*dims[0]*dims[1]*8 + ijk[1]*dims[0]*4 + ijk[0]*2;

        zind[0] = z_offset;
        zind[1] = z_offset + 1;
        zind[2] = z_offset + dims[0]*2;
        zind[3] = zind[2] + 1;

        for (int n = 0; n < 4; n++)
            zind[n+4] = zind[n] + dims[0]*dims[1]*4;


        for (int n = 0; n< 8; n++)
           Z[n] = m_zcorn[zind[n]];


        for (int  n=0; n<4; n++) {
            const double xt = m_coord[pind[n]];
            const double yt = m_coord[pind[n] + 1];
            const double zt = m_coord[pind[n] + 2];

            const double zb = m_coord[pind[n]+5];

            if (zt == zb) {
                X[n] = xt;
                X[n + 4] = xt;

                Y[n] = yt;
                Y[n + 4] = yt;
            } else {
                const double xb = m_coord[pind[n] + 3];
                const double yb = m_coord[pind[n] + 4];
                X[n] = xt + (xb-xt) / (zt-zb) * (zt - Z[n]);
                X[n+4] = xt + (xb-xt) / (zt-zb) * (zt-Z[n+4]);

                Y[n] = yt+(yb-yt)/(zt-zb)*(zt-Z[n]);
                Y[n+4] = yt+(yb-yt)/(zt-zb)*(zt-Z[n+4]);
            }
        }
    }


    void EclipseGrid::getCellCorners(const std::size_t globalIndex,
                                     std::array<double,8>& X,
                                     std::array<double,8>& Y,
                                     std::array<double,8>& Z) const
    {
        this->assertGlobalIndex(globalIndex);
        auto ijk = this->getIJK(globalIndex);
        this->getCellCorners(ijk, this->getNXYZ(), X, Y, Z);
    }


    std::vector<double> EclipseGrid::makeCoordDxvDyvDzvDepthz(const std::vector<double>& dxv, const std::vector<double>& dyv, const std::vector<double>& dzv, const std::vector<double>& depthz) const {
        auto nx = this->getNX();
        auto ny = this->getNY();
        auto nz = this->getNZ();

        std::vector<double> coord;
        coord.reserve((nx+1)*(ny+1)*6);

        std::vector<double> x(nx + 1, 0.0);
        std::partial_sum(dxv.begin(), dxv.end(), x.begin() + 1);

        std::vector<double> y(ny + 1, 0.0);
        std::partial_sum(dyv.begin(), dyv.end(), y.begin() + 1);

        std::vector<double> z(nz + 1, 0.0);
        std::partial_sum(dzv.begin(), dzv.end(), z.begin() + 1);


        for (std::size_t j = 0; j < ny + 1; j++) {
            for (std::size_t i = 0; i<nx + 1; i++) {

                const double x0 = x[i];
                const double y0 = y[j];

                std::size_t ind = i+j*(nx+1);

                double zb = depthz[ind] + z[nz];

                coord.push_back(x0);
                coord.push_back(y0);
                coord.push_back(depthz[ind]);
                coord.push_back(x0);
                coord.push_back(y0);
                coord.push_back(zb);
            }
        }

        return coord;
    }

    std::vector<double> EclipseGrid::makeZcornDzvDepthz(const std::vector<double>& dzv, const std::vector<double>& depthz) const {
        auto nx = this->getNX();
        auto ny = this->getNY();
        auto nz = this->getNZ();

        std::vector<double> zcorn;
        std::size_t sizeZcorn = nx*ny*nz*8;

        zcorn.assign (sizeZcorn, 0.0);

        std::vector<double> z(nz + 1, 0.0);
        std::partial_sum(dzv.begin(), dzv.end(), z.begin() + 1);


        for (std::size_t k = 0; k < nz; k++) {
            for (std::size_t j = 0; j < ny; j++) {
                for (std::size_t i = 0; i < nx; i++) {

                    const double z0 = z[k];

                    // top face of cell
                    auto zind = i*2 + j*nx*4 + k*nx*ny*8;

                    zcorn[zind] = depthz[i+j*(nx+1)] + z0;
                    zcorn[zind + 1] = depthz[i+j*(nx+1) +1 ] + z0;

                    zind = zind + nx*2;

                    zcorn[zind] = depthz[i+(j+1)*(nx+1)] + z0;
                    zcorn[zind + 1] = depthz[i+(j+1)*(nx+1) + 1] + z0;

                    // bottom face of cell
                    zind = i*2 + j*nx*4 + k*nx*ny*8 + nx*ny*4;

                    zcorn[zind] = depthz[i+j*(nx+1)] + z0 + dzv[k];
                    zcorn[zind + 1] = depthz[i+j*(nx+1) +1 ] + z0 + dzv[k];

                    zind = zind + nx*2;

                    zcorn[zind] = depthz[i+(j+1)*(nx+1)] + z0 + dzv[k];
                    zcorn[zind + 1] = depthz[i+(j+1)*(nx+1) + 1] + z0 + dzv[k];
                }
            }
        }

        return zcorn;
    }

    namespace
    {
        std::vector<double> makeSumIdirAtK(const int nx, const int ny, const int k, const std::vector<double>& dx)
        {
            std::vector<double> s(nx * ny, 0.0);
            for (int j = 0; j < ny; ++j) {
                double sum = 0.0;
                for (int i = 0; i < nx; ++i) {
                    sum += dx[i + j*nx + k*nx*ny];
                    s[i + j*nx] = sum;
                }
            }
            return s;
        }

        std::vector<double> makeSumJdirAtK(const int nx, const int ny, const int k, const std::vector<double>& dy)
        {
            std::vector<double> s(nx * ny, 0.0);
            for (int i = 0; i < nx; ++i) {
                double sum = 0.0;
                for (int j = 0; j < ny; ++j) {
                    sum += dy[i + j*nx + k*nx*ny];
                    s[i + j*nx] = sum;
                }
            }
            return s;
        }

        std::vector<double> makeSumKdir(const int nx, const int ny, const int nz, const std::vector<double>& dz)
        {
            std::vector<double> s(nx * ny, 0.0);
            for (int i = 0; i < nx; ++i) {
                for (int j = 0; j < ny; ++j) {
                    double sum = 0.0;
                    for (int k = 0; k < nz; ++k) {
                        sum += dz[i + j*nx + k*nx*ny];
                    }
                    s[i + j*nx] = sum;
                }
            }
            return s;
        }

    } // anonymous namespace

    std::vector<double> EclipseGrid::makeCoordDxDyDzTops(const std::vector<double>& dx, const std::vector<double>& dy, const std::vector<double>& dz, const std::vector<double>& tops) const {
        auto nx = this->getNX();
        auto ny = this->getNY();
        auto nz = this->getNZ();

        std::vector<double> coord;
        coord.reserve((nx+1)*(ny+1)*6);

        std::vector<double> sum_idir_top = makeSumIdirAtK(nx, ny, 0, dx);
        std::vector<double> sum_idir_bot = makeSumIdirAtK(nx, ny, nz - 1, dx);
        std::vector<double> sum_jdir_top = makeSumJdirAtK(nx, ny, 0, dy);
        std::vector<double> sum_jdir_bot = makeSumJdirAtK(nx, ny, nz - 1, dy);
        std::vector<double> sum_kdir = makeSumKdir(nx, ny, nz, dz);

        for (std::size_t j = 0; j < ny; j++) {

            double zt = tops[0];
            double zb = zt + sum_kdir[0 + 0*nx];

            if (j == 0) {
                double x0 = 0.0;
                double y0 = 0;

                coord.push_back(x0);
                coord.push_back(y0);
                coord.push_back(zt);
                coord.push_back(x0);
                coord.push_back(y0);
                coord.push_back(zb);

                for (std::size_t i = 0; i < nx; i++) {

                    std::size_t ind = i+j*nx+1;

                    if (i == (nx-1)) {
                        ind=ind-1;
                    }

                    zt = tops[ind];
                    zb = zt + sum_kdir[i + j*nx];

                    double xt = x0 + dx[i + j*nx];
                    double xb = sum_idir_bot[i + j*nx];

                    coord.push_back(xt);
                    coord.push_back(y0);
                    coord.push_back(zt);
                    coord.push_back(xb);
                    coord.push_back(y0);
                    coord.push_back(zb);

                    x0=xt;
                }
            }

            std::size_t ind = (j+1)*nx;

            if (j == (ny-1) ) {
                ind = j*nx;
            }

            double x0 = 0.0;

            double yt = sum_jdir_top[0 + j*nx];
            double yb = sum_jdir_bot[0 + j*nx];

            zt = tops[ind];
            zb = zt + sum_kdir[0 + j*nx];

            coord.push_back(x0);
            coord.push_back(yt);
            coord.push_back(zt);
            coord.push_back(x0);
            coord.push_back(yb);
            coord.push_back(zb);

            for (std::size_t i=0; i < nx; i++) {

                ind = i+(j+1)*nx+1;

                if (j == (ny-1) ) {
                    ind = i+j*nx+1;
                }

                if (i == (nx - 1) ) {
                    ind=ind-1;
                }

                zt = tops[ind];
                zb = zt + sum_kdir[i + j*nx];

                double xt=-999;
                double xb;

                if (j == (ny-1) ) {
                    xt = sum_idir_top[i + j*nx];
                    xb = sum_idir_bot[i + j*nx];
                } else {
                    xt = sum_idir_top[i + (j+1)*nx];
                    xb = sum_idir_bot[i + (j+1)*nx];
                }

                if (i == (nx - 1) ) {
                    yt = sum_jdir_top[i + j*nx];
                    yb = sum_jdir_bot[i + j*nx];
                } else {
                    yt = sum_jdir_top[(i + 1) + j*nx];
                    yb = sum_jdir_bot[(i + 1) + j*nx];
                }

                coord.push_back(xt);
                coord.push_back(yt);
                coord.push_back(zt);
                coord.push_back(xb);
                coord.push_back(yb);
                coord.push_back(zb);
            }
        }

        return coord;
    }

    std::vector<double> EclipseGrid::makeZcornDzTops(const std::vector<double>& dz, const std::vector<double>& tops) const {

        std::vector<double> zcorn;
        std::size_t sizeZcorn = this->getCartesianSize() * 8;
        auto nx = this->getNX();
        auto ny = this->getNY();
        auto nz = this->getNZ();
        zcorn.assign (sizeZcorn, 0.0);

        for (std::size_t j = 0; j < ny; j++) {
            for (std::size_t i = 0; i < nx; i++) {
                std::size_t ind = i + j*nx;
                double z = tops[ind];

                for (std::size_t k = 0; k < nz; k++) {

                    // top face of cell
                    std::size_t zind = i*2 + j*nx*4 + k*nx*ny*8;

                    zcorn[zind] = z;
                    zcorn[zind + 1] = z;

                    zind = zind + nx*2;

                    zcorn[zind] = z;
                    zcorn[zind + 1] = z;

                    z = z + dz[i + j*nx + k*nx*ny];

                    // bottom face of cell
                    zind = i*2 + j*nx*4 + k*nx*ny*8 + nx*ny*4;

                    zcorn[zind] = z;
                    zcorn[zind + 1] = z;

                    zind = zind + nx*2;

                    zcorn[zind] = z;
                    zcorn[zind + 1] = z;
                }
            }
        }

        return zcorn;
    }

    void EclipseGrid::initCylindricalGrid(const Deck& deck)
    {
        initSpiderwebOrCylindricalGrid(deck, true);
    }

    void EclipseGrid::initSpiderwebGrid(const Deck& deck)
    {
        initSpiderwebOrCylindricalGrid(deck, false);
    }

    /*
      Limited implementaton - requires keywords: DRV, DTHETAV, DZV or DZ, and TOPS.
    */

    void EclipseGrid::initSpiderwebOrCylindricalGrid(const Deck& deck, const bool is_cylindrical)
    {
        const std::string kind = is_cylindrical ? "cylindrical" : "spiderweb";

        // The hasCylindricalKeywords( ) checks according to the
        // eclipse specification for RADIAL grid. We currently do not support all
        // aspects of cylindrical grids, we therefor have an
        // additional test here, which checks if we have the keywords
        // required by the current implementation.
        if (!hasCylindricalKeywords(deck))
            throw std::invalid_argument("Not all keywords required for " + kind + " grids present");

        if (!deck.hasKeyword<ParserKeywords::DTHETAV>())
            throw std::logic_error("The current implementation *must* have theta values specified using the DTHETAV keyword");

        if (!deck.hasKeyword<ParserKeywords::DRV>())
            throw std::logic_error("The current implementation *must* have radial values specified using the DRV keyword");

        if (!(deck.hasKeyword<ParserKeywords::DZ>() || deck.hasKeyword<ParserKeywords::DZV>()) || !deck.hasKeyword<ParserKeywords::TOPS>())
            throw std::logic_error("The vertical cell size must be specified using the DZ or DZV, and the TOPS keywords");

        const std::vector<double>& drv     = deck.get<ParserKeywords::DRV>().back().getSIDoubleData();
        const std::vector<double>& dthetav = deck.get<ParserKeywords::DTHETAV>().back().getSIDoubleData();
        const std::vector<double>& tops    = deck.get<ParserKeywords::TOPS>().back().getSIDoubleData();
        OpmLog::info(fmt::format("\nCreating {} grid from keywords DRV, DTHETAV, DZV and TOPS", kind));

        if (drv.size() != this->getNX())
            throw std::invalid_argument("DRV keyword should have exactly " + std::to_string( this->getNX() ) + " elements");

        if (dthetav.size() != this->getNY())
            throw std::invalid_argument("DTHETAV keyword should have exactly " + std::to_string( this->getNY() ) + " elements");

        auto area = this->getNX() * this->getNY();
        std::size_t volume = this->getNX() * this->getNY() * this->getNZ();

        std::vector<double> dz(volume);
        if (deck.hasKeyword<ParserKeywords::DZ>()) {
            const std::vector<double>& dz_deck = deck.get<ParserKeywords::DZ>().back().getSIDoubleData();
            if (dz_deck.size() != volume)
                throw std::invalid_argument("DZ keyword should have exactly " + std::to_string( volume ) + " elements");
            dz = dz_deck;
        } else {
            const std::vector<double>& dzv = deck.get<ParserKeywords::DZV>().back().getSIDoubleData();
            if (dzv.size() != this->getNZ())
                throw std::invalid_argument("DZV keyword should have exactly " + std::to_string( this->getNZ() ) + " elements");
            for (std::size_t k= 0; k < this->getNZ(); k++)
                std::fill(dz.begin() + k*area, dz.begin() + (k+1)*area, dzv[k]);
        }

        if (tops.size() != (this->getNX() * this->getNY()))
            throw std::invalid_argument("TOPS keyword should have exactly " + std::to_string( this->getNX() * this->getNY() ) + " elements");

        {
            double total_angle = std::accumulate(dthetav.begin(), dthetav.end(), 0.0);

            if (std::abs( total_angle - 360 ) < 0.01)
                m_circle = deck.hasKeyword<ParserKeywords::CIRCLE>();
            else {
                if (total_angle > 360)
                    throw std::invalid_argument("More than 360 degrees rotation - cells will be double covered");
            }
        }

        /*
          Now the data has been validated, now we continue to create
          ZCORN and COORD vectors, and we are almost done.
        */
        {
            ZcornMapper zm( this->getNX(), this->getNY(), this->getNZ());
            CoordMapper cm(this->getNX(), this->getNY());
            std::vector<double> zcorn( zm.size() );
            std::vector<double> coord( cm.size() );
            {
                std::vector<double> depth(tops);
                for (std::size_t k = 0; k < this->getNZ(); k++) {
                    for (std::size_t j = 0; j < this->getNY(); j++) {
                        for (std::size_t i = 0; i < this->getNX(); i++) {
                            auto current_depth = depth[j * this->getNX() + i];
                            auto next_depth = current_depth + dz[k * area + j * this->getNX() + i];
                            for (std::size_t c=0; c < 4; c++) {
                                zcorn[ zm.index(i,j,k,c) ]     = current_depth;
                                zcorn[ zm.index(i,j,k,c + 4) ] = next_depth;
                            }
                            depth[j * this->getNX() + i] = next_depth;
                        }
                    }
                }
            }
            {
                std::vector<double> ri(this->getNX() + 1);
                std::vector<double> tj(this->getNY() + 1);
                double z1 = *std::min_element( zcorn.begin() , zcorn.end());
                double z2 = *std::max_element( zcorn.begin() , zcorn.end());
                ri[0] = deck.get<ParserKeywords::INRAD>().back().getRecord(0).getItem(0).getSIDouble( 0 );
                for (std::size_t i = 1; i <= this->getNX(); i++)
                    ri[i] = ri[i - 1] + drv[i - 1];

                tj[0] = 0;
                for (std::size_t j = 1; j <= this->getNY(); j++)
                    tj[j] = tj[j - 1] + dthetav[j - 1];


                for (std::size_t j = 0; j <= this->getNY(); j++) {
                    /*
                      The theta value is supposed to go counterclockwise, starting at 'twelve o clock'.
                    */
                    double t = M_PI * (90 - tj[j]) / 180;
                    double c = cos( t );
                    double s = sin( t );
                    for (std::size_t i = 0; i <= this->getNX(); i++) {
                        double r = ri[i];
                        double x = r*c;
                        double y = r*s;

                        coord[ cm.index(i,j,0,0) ] = x;
                        coord[ cm.index(i,j,1,0) ] = y;
                        coord[ cm.index(i,j,2,0) ] = z1;

                        coord[ cm.index(i,j,0,1) ] = x;
                        coord[ cm.index(i,j,1,1) ] = y;
                        coord[ cm.index(i,j,2,1) ] = z2;
                    }
                }

                // Save angles, used by the cylindrical grid to calculate volumes.
                if (is_cylindrical) {
                    m_rv = ri;
                    m_thetav = dthetav;
                }

            }
            initCornerPointGrid( coord, zcorn, nullptr);
        }
    }



    void EclipseGrid::initCornerPointGrid(const std::vector<double>& coord ,
                                          const std::vector<double>& zcorn ,
                                          const int * actnum)


    {
        m_coord = coord;
        m_zcorn = zcorn;

        m_input_coord = coord;
        m_input_zcorn = zcorn;

        ZcornMapper mapper( getNX(), getNY(), getNZ());
        zcorn_fixed = mapper.fixupZCORN( m_zcorn );
        this->resetACTNUM(actnum);
    }

    void EclipseGrid::initCornerPointGrid(const Deck& deck)
    {
        this->assertCornerPointKeywords(deck);

        OpmLog::info(fmt::format("\nCreating corner-point grid from "
                                 "keywords COORD, ZCORN and others"));

        const auto& coord = deck.get<ParserKeywords::COORD>().back();
        const auto& zcorn = deck.get<ParserKeywords::ZCORN>().back();

        this->initCornerPointGrid(coord.getSIDoubleData(),
                                  zcorn.getSIDoubleData(),
                                  nullptr);
    }

    bool EclipseGrid::hasCornerPointKeywords(const Deck& deck) {
        if (deck.hasKeyword<ParserKeywords::ZCORN>() && deck.hasKeyword<ParserKeywords::COORD>())
            return true;
        else
            return false;
    }


    void EclipseGrid::assertCornerPointKeywords(const Deck& deck)
    {
        const int nx = this->getNX();
        const int ny = this->getNY();
        const int nz = this->getNZ();
        {
            const auto& ZCORNKeyWord = deck.get<ParserKeywords::ZCORN>().back();

            if (ZCORNKeyWord.getDataSize() != static_cast<size_t>(8*nx*ny*nz)) {
                const std::string msg =
                    "Wrong size of the ZCORN keyword: Expected 8*nx*ny*nz = "
                    + std::to_string(static_cast<long long>(8*nx*ny*nz)) + " is "
                    + std::to_string(static_cast<long long>(ZCORNKeyWord.getDataSize()));
                OpmLog::error(msg);
                throw std::invalid_argument(msg);
            }
        }

        {
            const auto& COORDKeyWord = deck.get<ParserKeywords::COORD>().back();
            if (COORDKeyWord.getDataSize() != static_cast<size_t>(6*(nx + 1)*(ny + 1))) {
                const std::string msg =
                    "Wrong size of the COORD keyword: Expected 6*(nx + 1)*(ny + 1) = "
                    + std::to_string(static_cast<long long>(6*(nx + 1)*(ny + 1))) + " is "
                    + std::to_string(static_cast<long long>(COORDKeyWord.getDataSize()));
                OpmLog::error(msg);
                throw std::invalid_argument(msg);
            }
        }
    }


    bool EclipseGrid::hasGDFILE(const Deck& deck) {
        return deck.hasKeyword<ParserKeywords::GDFILE>();
    }


    bool EclipseGrid::hasCartesianKeywords(const Deck& deck) {
        if (hasDVDEPTHZKeywords( deck ))
            return true;
        else
            return hasDTOPSKeywords(deck);
    }


    bool EclipseGrid::hasRadialKeywords(const Deck& deck) {
        return deck.hasKeyword<ParserKeywords::RADIAL>() && hasCylindricalKeywords(deck);
    }


    bool EclipseGrid::hasSpiderKeywords(const Deck& deck) {
        return deck.hasKeyword<ParserKeywords::SPIDER>() && hasCylindricalKeywords(deck);
    }


    bool EclipseGrid::hasCylindricalKeywords(const Deck& deck) {
        if (deck.hasKeyword<ParserKeywords::INRAD>() &&
            deck.hasKeyword<ParserKeywords::TOPS>() &&
            (deck.hasKeyword<ParserKeywords::DZ>() || deck.hasKeyword<ParserKeywords::DZV>()) &&
            (deck.hasKeyword<ParserKeywords::DRV>() || deck.hasKeyword<ParserKeywords::DR>()) &&
            (deck.hasKeyword<ParserKeywords::DTHETA>() || deck.hasKeyword<ParserKeywords::DTHETAV>()))
            return true;
        else
            return false;
    }


    bool EclipseGrid::hasDVDEPTHZKeywords(const Deck& deck) {
        if (deck.hasKeyword<ParserKeywords::DXV>() &&
            deck.hasKeyword<ParserKeywords::DYV>() &&
            deck.hasKeyword<ParserKeywords::DZV>() &&
            deck.hasKeyword<ParserKeywords::DEPTHZ>())
            return true;
        else
            return false;
    }

    bool EclipseGrid::hasEqualDVDEPTHZ(const Deck& deck) {
        const std::vector<double>& DXV = deck.get<ParserKeywords::DXV>().back().getSIDoubleData();
        const std::vector<double>& DYV = deck.get<ParserKeywords::DYV>().back().getSIDoubleData();
        const std::vector<double>& DZV = deck.get<ParserKeywords::DZV>().back().getSIDoubleData();
        const std::vector<double>& DEPTHZ = deck.get<ParserKeywords::DEPTHZ>().back().getSIDoubleData();
        if (EclipseGrid::allEqual(DXV) &&
            EclipseGrid::allEqual(DYV)&&
            EclipseGrid::allEqual(DZV)&&
            EclipseGrid::allEqual(DEPTHZ))
            return true;
        else
            return false;
    }

    bool EclipseGrid::hasDTOPSKeywords(const Deck& deck) {
        if ((deck.hasKeyword<ParserKeywords::DX>() || deck.hasKeyword<ParserKeywords::DXV>()) &&
            (deck.hasKeyword<ParserKeywords::DY>() || deck.hasKeyword<ParserKeywords::DYV>()) &&
            (deck.hasKeyword<ParserKeywords::DZ>() || deck.hasKeyword<ParserKeywords::DZV>()) &&
            deck.hasKeyword<ParserKeywords::TOPS>())
            return true;
        else
            return false;
    }

    void EclipseGrid::assertVectorSize(const std::vector<double>& vector , std::size_t expectedSize , const std::string& vectorName) {
        if (vector.size() != expectedSize)
            throw std::invalid_argument("Wrong size for keyword: " + vectorName + ". Expected: " + std::to_string(expectedSize) + " got: " + std::to_string(vector.size()));
    }


    /*
      The body of the for loop in this method looks slightly
      peculiar. The situation is as follows:

        1. The EclipseGrid class will assemble the necessary keywords
           and create an ert ecl_grid instance.

        2. The ecl_grid instance will export ZCORN, COORD and ACTNUM
           data which will be used by the UnstructureGrid constructor
           in opm-core. If the ecl_grid is created with ZCORN as an
           input keyword that data is retained in the ecl_grid
           structure, otherwise the ZCORN data is created based on the
           internal cell geometries.

        3. When constructing the UnstructuredGrid structure strict
           numerical comparisons of ZCORN values are used to detect
           cells in contact, if all the elements the elements in the
           TOPS vector are specified[1] we will typically not get
           bitwise equality between the bottom of one cell and the top
           of the next.

       To remedy this we enforce bitwise equality with the
       construction:

          if (std::abs(nextValue - TOPS[targetIndex]) < z_tolerance)
             TOPS[targetIndex] = nextValue;

       [1]: This is of course assuming the intention is to construct a
            fully connected space covering grid - if that is indeed
            not the case the barriers must be thicker than 1e-6m to be
            retained.
    */

    std::vector<double> EclipseGrid::createTOPSVector(const std::array<int, 3>& dims,
            const std::vector<double>& DZ, const Deck& deck)
    {
        std::size_t volume = dims[0] * dims[1] * dims[2];
        std::size_t area = dims[0] * dims[1];
        const auto& TOPSKeyWord = deck.get<ParserKeywords::TOPS>().back();
        std::vector<double> TOPS = TOPSKeyWord.getSIDoubleData();

        if (TOPS.size() >= area) {
            std::size_t initialTOPSize = TOPS.size();
            TOPS.resize( volume );

            for (std::size_t targetIndex = area; targetIndex < volume; targetIndex++) {
                std::size_t sourceIndex = targetIndex - area;
                double nextValue = TOPS[sourceIndex] + DZ[sourceIndex];

                if (targetIndex >= initialTOPSize)
                    TOPS[targetIndex] = nextValue;
                else {
                    constexpr double z_tolerance = 1e-6;
                    if (std::abs(nextValue - TOPS[targetIndex]) < z_tolerance)
                        TOPS[targetIndex] = nextValue;
                }

            }
        }

        if (TOPS.size() != volume)
            throw std::invalid_argument("TOPS size mismatch");

        return TOPS;
    }

std::vector<double> EclipseGrid::createDVector(const std::array<int,3>& dims, std::size_t dim, const std::string& DKey,
            const std::string& DVKey, const Deck& deck)
    {
        std::size_t volume = dims[0] * dims[1] * dims[2];
        std::vector<double> D;
        if (deck.hasKeyword(DKey)) {
            D = deck[DKey].back().getSIDoubleData();

            const std::size_t area = dims[0] * dims[1];
            if (D.size() >= area && D.size() < volume) {
                /*
                  Only the top layer is required; for layers below the
                  top layer the value from the layer above is used.
                */
                std::size_t initialDSize = D.size();
                D.resize( volume );
                for (std::size_t targetIndex = initialDSize; targetIndex < volume; targetIndex++) {
                    std::size_t sourceIndex = targetIndex - area;
                    D[targetIndex] = D[sourceIndex];
                }
            }

            if (D.size() != volume)
                throw std::invalid_argument(DKey + " size mismatch");
        } else {
            const auto& DVKeyWord = deck[DVKey].back();
            const std::vector<double>& DV = DVKeyWord.getSIDoubleData();
            if (DV.size() != static_cast<std::size_t>(dims[dim]))
                throw std::invalid_argument(DVKey + " size mismatch");
            D.resize( volume );
            scatterDim( dims , dim , DV , D );
        }
        return D;
    }


    void EclipseGrid::scatterDim(const std::array<int, 3>& dims , std::size_t dim , const std::vector<double>& DV , std::vector<double>& D) {
        int index[3];
        for (index[2] = 0;  index[2] < dims[2]; index[2]++) {
            for (index[1] = 0; index[1] < dims[1]; index[1]++) {
                for (index[0] = 0;  index[0] < dims[0]; index[0]++) {
                    std::size_t globalIndex = index[2] * dims[1] * dims[0] + index[1] * dims[0] + index[0];
                    D[globalIndex] = DV[ index[dim] ];
                }
            }
        }
    }

    bool EclipseGrid::allEqual(const std::vector<double> &v) {
        auto comp = [](double x, double y) { return std::fabs(x - y) < 1e-12; };
        return std::equal(v.begin() + 1, v.end(), v.begin(), comp);
    }

    bool EclipseGrid::equal(const EclipseGrid& other) const {

        //double reltol = 1.0e-6;

        if (m_coord.size() != other.m_coord.size())
            return false;

        if (m_zcorn.size() != other.m_zcorn.size())
            return false;

        if (!(m_mapaxes == other.m_mapaxes))
            return false;

        if (m_actnum != other.m_actnum)
            return false;

        if (m_coord != other.m_coord)
            return false;

        if (m_zcorn != other.m_zcorn)
            return false;

        bool status = ((m_pinch == other.m_pinch)  && (m_minpvMode == other.getMinpvMode()));

        if (m_minpvMode != MinpvMode::Inactive) {
            status = status && (m_minpvVector == other.getMinpvVector());
        }

        return status;
    }

    std::size_t EclipseGrid::getNumActive( ) const {
        return this->m_nactive;
    }

    bool EclipseGrid::allActive( ) const {
        return (getNumActive() == getCartesianSize());
    }

    bool EclipseGrid::cellActive( std::size_t globalIndex ) const {
        assertGlobalIndex( globalIndex );
        if (m_actnum.empty()) {
            return true;
        } else {
            return m_actnum[globalIndex]>0;
        }
    }

    bool EclipseGrid::cellActive( std::size_t i , std::size_t j , std::size_t k ) const {
        assertIJK(i,j,k);

        std::size_t globalIndex = getGlobalIndex(i,j,k);
        return this->cellActive(globalIndex);
    }

    bool EclipseGrid::cellActiveAfterMINPV( std::size_t i , std::size_t j , std::size_t k, double cell_porv ) const {
        assertIJK(i,j,k);

        std::size_t globalIndex = getGlobalIndex(i,j,k);
        if (!this->cellActive(globalIndex))
            return false;

        return m_minpvMode == MinpvMode::Inactive || cell_porv >= m_minpvVector[globalIndex];
    }

    const std::vector<double>& EclipseGrid::activeVolume() const {
        if (!this->active_volume.has_value()) {
            std::vector<double> volume(this->m_nactive);

            #pragma omp parallel for schedule(static)
            for (std::int64_t active_index = 0; active_index < static_cast<std::int64_t>(this->m_active_to_global.size()); active_index++) {
                std::array<double,8> X;
                std::array<double,8> Y;
                std::array<double,8> Z;
                auto global_index = this->m_active_to_global[active_index];
                this->getCellCorners(global_index, X, Y, Z );
                if (m_rv && m_thetav) {
                    const auto[i,j,k] = this->getIJK(global_index);
                    const auto& r = *m_rv;
                    const auto& t = *m_thetav;
                    volume[active_index] = calculateCylindricalCellVol(r[i], r[i+1], t[j], Z[4] - Z[0]);
                } else
                    volume[active_index] = calculateCellVol(X, Y, Z);
            }

            this->active_volume = std::move(volume);
        }

        return this->active_volume.value();
    }


    double EclipseGrid::getCellVolume(std::size_t globalIndex) const {
        assertGlobalIndex( globalIndex );
        if (this->cellActive(globalIndex) && this->active_volume.has_value()) {
            auto active_index = this->activeIndex(globalIndex);
            return this->active_volume.value()[active_index];
        }

        std::array<double,8> X;
        std::array<double,8> Y;
        std::array<double,8> Z;
        this->getCellCorners(globalIndex, X, Y, Z );
        if (m_rv && m_thetav) {
            const auto[i,j,k] = this->getIJK(globalIndex);
            const auto& r = *m_rv;
            const auto& t = *m_thetav;
           return calculateCylindricalCellVol(r[i], r[i+1], t[j], Z[4] - Z[0]);
        } else {
            return calculateCellVol(X, Y, Z);
        }
    }

    double EclipseGrid::getCellVolume(std::size_t i , std::size_t j , std::size_t k) const {
        this->assertIJK(i,j,k);
        std::size_t globalIndex = getGlobalIndex(i,j,k);
        return this->getCellVolume(globalIndex);
    }

    double EclipseGrid::getCellThickness(std::size_t i , std::size_t j , std::size_t k) const {
        assertIJK(i,j,k);

        const std::array<int, 3> dims = getNXYZ();
        std::size_t globalIndex = i + j*dims[0] + k*dims[0]*dims[1];

        return getCellThickness(globalIndex);
    }

    double EclipseGrid::getCellThickness(std::size_t globalIndex) const {
        assertGlobalIndex( globalIndex );
        std::array<double,8> X;
        std::array<double,8> Y;
        std::array<double,8> Z;
        this->getCellCorners(globalIndex, X, Y, Z );

        double z2 = (Z[4]+Z[5]+Z[6]+Z[7])/4.0;
        double z1 = (Z[0]+Z[1]+Z[2]+Z[3])/4.0;
        double dz = z2-z1;
        return dz;
    }


    std::array<double, 3> EclipseGrid::getCellDims(std::size_t globalIndex) const {
        assertGlobalIndex( globalIndex );
        std::array<double,8> X;
        std::array<double,8> Y;
        std::array<double,8> Z;
        this->getCellCorners(globalIndex, X, Y, Z );

        // calculate dx
        double x1 = (X[0]+X[2]+X[4]+X[6])/4.0;
        double y1 = (Y[0]+Y[2]+Y[4]+Y[6])/4.0;
        double x2 = (X[1]+X[3]+X[5]+X[7])/4.0;
        double y2 = (Y[1]+Y[3]+Y[5]+Y[7])/4.0;
        double dx = sqrt(pow((x2-x1), 2.0) + pow((y2-y1), 2.0) );

        // calculate dy
        x1 = (X[0]+X[1]+X[4]+X[5])/4.0;
        y1 = (Y[0]+Y[1]+Y[4]+Y[5])/4.0;
        x2 = (X[2]+X[3]+X[6]+X[7])/4.0;
        y2 = (Y[2]+Y[3]+Y[6]+Y[7])/4.0;
        double dy = sqrt(pow((x2-x1), 2.0) + pow((y2-y1), 2.0));

        // calculate dz

        double z2 = (Z[4]+Z[5]+Z[6]+Z[7])/4.0;
        double z1 = (Z[0]+Z[1]+Z[2]+Z[3])/4.0;
        double dz = z2-z1;


        return std::array<double,3> {{dx, dy, dz}};
    }

    std::array<double, 3> EclipseGrid::getCellDims(std::size_t i , std::size_t j , std::size_t k) const {
        assertIJK(i,j,k);
        {
            std::size_t globalIndex = getGlobalIndex( i,j,k );

            return getCellDims(globalIndex);
        }
    }


    std::tuple<std::array<double, 3>,std::array<double, 3>,std::array<double, 3>>
    EclipseGrid::getCellAndBottomCenterNormal(std::size_t globalIndex) const {
        // bottom face is spanned by corners 4, ..., 7 and connects cells
        // with vertical index k and k+1
        assertGlobalIndex( globalIndex );
        std::array<double,8> X;
        std::array<double,8> Y;
        std::array<double,8> Z;
        this->getCellCorners(globalIndex, X, Y, Z );
        std::array<double,3> bottomCenter{{std::accumulate(X.begin() + 4, X.end(), 0.0) / 4.0,
                                           std::accumulate(Y.begin() + 4, Y.end(), 0.0) / 4.0,
                                           std::accumulate(Z.begin() + 4, Z.end(), 0.0) / 4.0}};

        // Calculate normal scaled with area via the triangles spanned by the center and two neighboring
        // corners.
        std::array<double,3> bottomFaceNormal{};
        // Reorder counter-clock.wise
        std::array<std::size_t,4> bottomIndices = {4, 5, 7, 6};
        std::array<double,3> oldCorner{ *(X.begin() + 6),
                                        *(Y.begin() + 6),
                                        *(Z.begin() + 6)};

        for(const auto cornerIndex: bottomIndices)
        {
            std::array<double,3> newCorner{ *(X.begin() + cornerIndex), *(Y.begin() + cornerIndex),
                                            *(Z.begin() + cornerIndex) };
            std::array<double,3> side1, side2, normalTriangle;
            std::ranges::transform(oldCorner, bottomCenter,
                                   side1.begin(), std::minus<double>());
            std::ranges::transform(newCorner, bottomCenter,
                                   side2.begin(), std::minus<double>());

            cross(side1, side2, normalTriangle);
            // use std::plus to make normal point outwards (Z-axis points downwards)
            std::ranges::transform(bottomFaceNormal, normalTriangle,
                                   bottomFaceNormal.begin(), std::plus<double>());
            oldCorner = newCorner;
        }

        std::ranges::transform(bottomFaceNormal, bottomFaceNormal.begin(),
                               [](const double& a){ return 0.5 * a;});
        std::array<double,3> cellCenter = { std::accumulate(X.begin(), X.end(), 0.0) / 8.0,
                                             std::accumulate(Y.begin(), Y.end(), 0.0) / 8.0,
                                             std::accumulate(Z.begin(), Z.end(), 0.0) / 8.0 };
            return std::make_tuple(cellCenter,
                                   bottomCenter,
                                   bottomFaceNormal);
    }

    std::array<double, 3> EclipseGrid::getCellCenter(std::size_t globalIndex) const {
        assertGlobalIndex( globalIndex );
        std::array<double,8> X;
        std::array<double,8> Y;
        std::array<double,8> Z;
        this->getCellCorners(globalIndex, X, Y, Z );
        return std::array<double,3> { { std::accumulate(X.begin(), X.end(), 0.0) / 8.0,
                                        std::accumulate(Y.begin(), Y.end(), 0.0) / 8.0,
                                        std::accumulate(Z.begin(), Z.end(), 0.0) / 8.0 } };
    }


    std::array<double, 3> EclipseGrid::getCellCenter(std::size_t i,std::size_t j, std::size_t k) const {
        assertIJK(i,j,k);

        const std::array<int, 3> dims = getNXYZ();

        std::size_t globalIndex = i + j*dims[0] + k*dims[0]*dims[1];

        return getCellCenter(globalIndex);
    }

    /*
      This is the numbering of the corners in the cell.

       bottom                           j
         6---7                        /|\
         |   |                         |
         4---5                         |
                                       |
       top                             o---------->  i
         2---3
         |   |
         0---1

     */

    std::array<double, 3> EclipseGrid::getCornerPos(std::size_t i, std::size_t j, std::size_t k, std::size_t corner_index) const {
        assertIJK(i,j,k);
        if (corner_index >= 8)
            throw std::invalid_argument("Invalid corner position");
        {
            const std::array<int, 3> dims = getNXYZ();

            std::array<double,8> X = {0.0};
            std::array<double,8> Y = {0.0};
            std::array<double,8> Z = {0.0};

            std::array<int, 3> ijk;

            ijk[0] = i;
            ijk[1] = j;
            ijk[2] = k;

            getCellCorners(ijk, dims, X, Y, Z );

            return std::array<double, 3> {{X[corner_index], Y[corner_index], Z[corner_index]}};
        }
    }

    bool EclipseGrid::isValidCellGeomtry(const std::size_t globalIndex,
                                         const UnitSystem& usys) const
    {
        const auto threshold = usys.to_si(UnitSystem::measure::length, 1.0e+20f);

        auto is_finite = [threshold](const double coord_component)
        {
            return std::abs(coord_component) < threshold;
        };

        std::array<double,8> X, Y, Z;
        this->getCellCorners(globalIndex, X, Y, Z);

        const auto finite_coord = std::all_of(X.begin(), X.end(), is_finite)
            && std::all_of(Y.begin(), Y.end(), is_finite)
            && std::all_of(Z.begin(), Z.end(), is_finite);

        if (! finite_coord) {
            return false;
        }

        const auto max_pillar_point_distance = std::max({
            Z[0 + 4] - Z[0 + 0],
            Z[1 + 4] - Z[1 + 0],
            Z[2 + 4] - Z[2 + 0],
            Z[3 + 4] - Z[3 + 0],
        });

        // Define points as "well separated" if maximum distance exceeds
        // 1e-4 length units (e.g., 0.1 mm in METRIC units).  May consider
        // using a coarser tolerance/threshold here.
        return max_pillar_point_distance
            >  usys.to_si(UnitSystem::measure::length, 1.0e-4);
    }

    double EclipseGrid::getCellDepth(std::size_t globalIndex) const {
        assertGlobalIndex( globalIndex );

        auto it = this->m_aquifer_cell_depths.find(globalIndex);
        return it != this->m_aquifer_cell_depths.end() ? it->second : computeCellGeometricDepth(globalIndex);
    }

    double EclipseGrid::computeCellGeometricDepth(std::size_t globalIndex) const {
        std::array<double,8> X;
        std::array<double,8> Y;
        std::array<double,8> Z;
        this->getCellCorners(globalIndex, X, Y, Z );

        double z2 = (Z[4]+Z[5]+Z[6]+Z[7])/4.0;
        double z1 = (Z[0]+Z[1]+Z[2]+Z[3])/4.0;
        return (z1 + z2)/2.0;
    }

    double EclipseGrid::getCellDepth(std::size_t i, std::size_t j, std::size_t k) const {
        this->assertIJK(i,j,k);
        std::size_t globalIndex = getGlobalIndex(i,j,k);
        return this->getCellDepth(globalIndex);
    }

    const std::map<std::size_t, std::array<int,2>>& EclipseGrid::getAquiferCellTabnums() const {
        return m_aquifer_cell_tabnums;
    }

    const std::vector<int>& EclipseGrid::getACTNUM( ) const {

        return m_actnum;
    }

    const std::vector<double>& EclipseGrid::getCOORD() const {

        return m_coord;
    }

    std::size_t EclipseGrid::fixupZCORN() {

        ZcornMapper mapper( getNX(), getNY(), getNZ());

        return mapper.fixupZCORN( m_zcorn );
    }

    const std::vector<double>& EclipseGrid::getZCORN( ) const {

        return m_zcorn;
    }

    void EclipseGrid::save_children(Opm::EclIO::EclOutput& egridfile, const Opm::UnitSystem& units) const {
        for (std::size_t index : m_print_order_lgr_cells) {
            lgr_children_cells[index].save(egridfile, units);
        }
    }

    void EclipseGrid::save(const std::string& filename, bool formatted, const std::vector<Opm::NNCdata>& nnc, const Opm::UnitSystem& units) const {
        Opm::EclIO::EclOutput egridfile(filename, formatted);
        save_core(egridfile, units);
        save_children(egridfile, units);
        save_nnc(egridfile, nnc);
    }

    void EclipseGrid::save_nnc(Opm::EclIO::EclOutput& egridfile, const std::vector<Opm::NNCdata>& nnc) const {
        std::vector<int> nnchead(10, 0);
        std::vector<int> nnc1;
        std::vector<int> nnc2;

        for (const NNCdata& n : nnc ) {
            nnc1.push_back(n.cell1 + 1);
            nnc2.push_back(n.cell2 + 1);
        }

        nnchead[0] = nnc1.size();
        if (nnc1.size() > 0){
            egridfile.write("NNCHEAD", nnchead);
            egridfile.write("NNC1", nnc1);
            egridfile.write("NNC2", nnc2);

        }

    }


    void EclipseGrid::save_core(Opm::EclIO::EclOutput& egridfile, const Opm::UnitSystem& units) const {

        Opm::UnitSystem::UnitType unitSystemType = units.getType();
        constexpr auto length = ::Opm::UnitSystem::measure::length;

        const std::array<int, 3> dims = getNXYZ();

        // Preparing vectors to be saved

        // create coord vector of floats with input units, converted from SI
        std::vector<float> coord_f;
        coord_f.resize(m_coord.size());

        auto convert_length = [&units](const double x) { return static_cast<float>(units.from_si(length, x)); };

        if (m_input_coord.has_value()) {
            std::ranges::transform(m_input_coord.value(), coord_f.begin(), convert_length);
        } else {
            std::ranges::transform(m_coord, coord_f.begin(), convert_length);
        }

        // create zcorn vector of floats with input units, converted from SI
        std::vector<float> zcorn_f;
        zcorn_f.resize(m_zcorn.size());

        if (m_input_zcorn.has_value()) {
            std::ranges::transform(m_input_zcorn.value(), zcorn_f.begin(), convert_length);
        } else {
            std::ranges::transform(m_zcorn, zcorn_f.begin(), convert_length);
        }

        m_input_coord.reset();
        m_input_zcorn.reset();

        std::vector<int> filehead(100,0);
        filehead[0] = 3;                     // version number
        filehead[1] = 2007;                  // release year
        filehead[6] = 1;                     // corner point grid

        egridfile.write("FILEHEAD", filehead);

        std::vector<int> gridhead(100,0);
        gridhead[0] = 1;                    // corner point grid
        gridhead[1] = dims[0];              // nI
        gridhead[2] = dims[1];              // nJ
        gridhead[3] = dims[2];              // nK
        gridhead[24] = 1;                   // NUMRES (number of reservoirs)
        //gridhead[25] = 1;                 // TODO: This value depends on LGRs?

        std::vector<std::string> gridunits;

        switch (unitSystemType) {
        case Opm::UnitSystem::UnitType::UNIT_TYPE_METRIC:
            gridunits.push_back("METRES");
            break;
        case Opm::UnitSystem::UnitType::UNIT_TYPE_FIELD:
            gridunits.push_back("FEET");
            break;
        case Opm::UnitSystem::UnitType::UNIT_TYPE_LAB:
            gridunits.push_back("CM");
            break;
        default:
            OPM_THROW(std::runtime_error, "Unit system not supported when writing to EGRID file");
            break;
        }
        gridunits.push_back("");

        std::vector<int> endgrid = {};

        // Writing vectors to egrid file

        if (this->m_mapaxes.has_value()) {
            const auto& mapunits = this->m_mapaxes.value().mapunits();
            if (mapunits.has_value())
                egridfile.write("MAPUNITS", std::vector<std::string>{ mapunits.value() });

            egridfile.write("MAPAXES", this->m_mapaxes.value().input());
        }

        egridfile.write("GRIDUNIT", gridunits);
        egridfile.write("GRIDHEAD", gridhead);

        egridfile.write("COORD", coord_f);
        egridfile.write("ZCORN", zcorn_f);

        egridfile.write("ACTNUM", m_actnum);
        egridfile.write("ENDGRID", endgrid);

    }

    EclipseGridLGR& EclipseGrid::getLGRCell(std::size_t index){
        return lgr_children_cells[index];
      }

    const EclipseGridLGR& EclipseGrid::getLGRCell(std::size_t index) const{
        return lgr_children_cells[index];
    }

     const EclipseGridLGR& EclipseGrid::getLGRCell(const std::string& lgr_tag) const
    {
        std::optional<std::reference_wrapper<const EclipseGridLGR>>lgr_found = std::nullopt;
        for (const auto& lgr_cell : lgr_children_cells)
        {
            const auto& lgr_temp = lgr_cell.get_child_LGR_cell(lgr_tag);
            if (lgr_temp != std::nullopt)
                lgr_found  = lgr_temp;
        }
        if (lgr_found == std::nullopt)
        {
            throw std::runtime_error("No EclipseGridLGR found with tag: " + lgr_tag);
        }
        else
        {
            return lgr_found.value().get();
        }
    }

    // @brief Recursively finds the global father index of a local grid refinement (LGR) cell.
    int EclipseGrid::getLGR_global_father(std::size_t global_index,  const std::string& lgr_tag) const
    {
        const EclipseGridLGR& lgr_cell = getLGRCell(lgr_tag);
        const std::string& father_label  = lgr_cell.get_father_label();
        if (father_label == "GLOBAL")
        {
            return lgr_cell.get_hostnum(global_index);
        }
        else
        {
            return getLGR_global_father(lgr_cell.get_hostnum(global_index), father_label);
        }
    }

    // @brief Recursively finds the father index of a local grid refinement (LGR) cell.
    int EclipseGrid::getLGR_father(std::size_t i, std::size_t j, std::size_t k, const std::string& lgr_tag) const {
        const EclipseGridLGR& lgr_cell = getLGRCell(lgr_tag);
        lgr_cell.assertIJK(i, j, k);
        std::size_t global_index = lgr_cell.getGlobalIndex(i, j, k);
        return getLGR_father(global_index, lgr_tag);
    }

    int EclipseGrid::getLGR_father(std::size_t global_index, const std::string& lgr_tag) const {
        const EclipseGridLGR& lgr_cell = getLGRCell(lgr_tag);
        return lgr_cell.get_hostnum(global_index);
    }

    std::array<int,3> EclipseGrid::getLGR_fatherIJK(std::size_t i, std::size_t j, std::size_t k, const std::string& lgr_tag) const {
        int global_id = getLGR_father(i, j, k, lgr_tag);
        return this->getIJK(global_id);
    }

    std::array<int,3> EclipseGrid::getCellSubdivisionRatioLGR(const std::string& lgr_tag,
                                                                    std::array<int,3> acum ) const
    {
        const EclipseGridLGR& lgr_cell = getLGRCell(lgr_tag);
        const std::string& father_label  = lgr_cell.get_father_label();
        acum = {static_cast<int>(acum[0]*lgr_cell.getNX()),
                static_cast<int>(acum[1]*lgr_cell.getNY()),
                static_cast<int>(acum[2]*lgr_cell.getNZ())};
        if (father_label == "GLOBAL")
        {
            return acum;
        }
        else
        {
            return getCellSubdivisionRatioLGR(father_label, acum);
        }
    }


    std::vector<GridDims> EclipseGrid::get_lgr_children_gridim() const
    {
        std::vector<GridDims> lgr_children_gridim;
        for (const std::string& lgr_tag : get_all_lgr_labels())
        {
            const EclipseGridLGR& lgr_cell =  getLGRCell(lgr_tag);
            lgr_children_gridim.emplace_back(lgr_cell.getNX(), lgr_cell.getNY(), lgr_cell.getNZ());
        }
        return lgr_children_gridim;
    }

    void EclipseGrid::set_lgr_refinement(const std::string& lgr_tag, const std::vector<double>& coords,
                                                                            const std::vector<double> & zcorn){
        for (auto& lgr_cell : lgr_children_cells) {
            lgr_cell.set_lgr_refinement(lgr_tag, coords, zcorn);
        }
    }


    void EclipseGrid::init_children_host_cells(bool logical){
        if (logical)
            init_children_host_cells_logical();
        else
            init_children_host_cells_geometrical();
    }

    void EclipseGrid::init_children_host_cells_geometrical(){
        auto getAllCellCorners = [this](const auto& father_list){
            std::vector<std::array<double, 8>> X(father_list.size());
            std::vector<std::array<double, 8>> Y(father_list.size());
            std::vector<std::array<double, 8>> Z(father_list.size());
            for (std::size_t index = 0; index < father_list.size(); index++) {
                getCellCorners(father_list[index],X[index],Y[index],Z[index]);
            }
            return std::make_tuple(X, Y,Z);
        };

        std::vector<double> element_centerX, element_centerY, element_centerZ;
        for (EclipseGridLGR& lgr_cell : lgr_children_cells) {
            std::tie(element_centerX, element_centerY,element_centerZ) =
                VectorUtil::callMethodForEachInputOnObjectXYZ<EclipseGridLGR, std::array<double,3>, int,std::array<double, 3> (EclipseGridLGR::*)(size_t) const>
                (lgr_cell, &EclipseGridLGR::getCellCenter, lgr_cell.getActiveMap());
            auto [host_cellX, host_cellY, host_cellZ]  =  getAllCellCorners(lgr_cell.get_father_global());
            auto inside_el = GeometryUtil::isInsideElement(element_centerX, element_centerY, element_centerZ, host_cellX, host_cellY, host_cellZ);
            std::vector<int> host_cells_global_ref = VectorUtil::filterArray<int>(lgr_cell.get_father_global(), inside_el);
            lgr_cell.set_hostnum(host_cells_global_ref);
            lgr_cell.init_children_host_cells();
        }
    }


    void EclipseGrid::init_children_host_cells_logical(){
        auto  IJK_location = [](const std::size_t&  nx, const std::size_t& ny,const std::size_t& nz,
                                          const std::size_t& host_nx, const std::size_t& host_ny, const std::size_t& host_nz,
                                          const std::size_t& base_host_nx, const std::size_t& base_host_ny, const std::size_t& base_host_nz){
            const auto [i_list, j_list, k_list] = VectorUtil::generate_cartesian_product(0, nx-1, 0, ny-1, 0, nz-1);
            std::vector<std::size_t> resultI = VectorUtil::scalarVectorOperation(nx/host_nx, i_list,  std::divides<std::size_t>{});
            resultI = VectorUtil::vectorScalarOperation(resultI, base_host_nx, std::plus<std::size_t>{});
            std::vector<std::size_t> resultJ = VectorUtil::scalarVectorOperation(ny/host_ny, j_list,  std::divides<std::size_t>{});
            resultJ = VectorUtil::vectorScalarOperation(resultJ, base_host_ny , std::plus<std::size_t>{});
            std::vector<std::size_t> resultK = VectorUtil::scalarVectorOperation(nz/host_nz, k_list,  std::divides<std::size_t>{});
            resultK = VectorUtil::vectorScalarOperation(resultK, base_host_nz, std::plus<std::size_t>{});
            return std::make_tuple(resultI, resultJ, resultK);
        };
        auto getAllGlobalIndex  = [this](const std::vector<std::size_t>& i_list,
                                                   const std::vector<std::size_t>& j_list,
                                                   const std::vector<std::size_t>& k_list){
            std::vector<int> global_index(i_list.size());
            for (std::size_t idx = 0; idx < i_list.size(); ++idx) {
                global_index[idx] = getGlobalIndex(i_list[idx], j_list[idx], k_list[idx]);
            }
            return global_index;
        };
        for (EclipseGridLGR& lgr_cell : lgr_children_cells) {
            const std::array<int,3>& host_low_fatherIJK = lgr_cell.get_low_fatherIJK();
            const std::array<int,3>& host_up_fatherIJK = lgr_cell.get_up_fatherIJK();
            const std::array<int,3> host_IJK = {host_up_fatherIJK[0] - host_low_fatherIJK[0] + 1,
                                                host_up_fatherIJK[1] - host_low_fatherIJK[1] + 1,
                                                host_up_fatherIJK[2] - host_low_fatherIJK[2] + 1};
            auto [i_list, j_list, k_list] = IJK_location(lgr_cell.getNX(), lgr_cell.getNY(), lgr_cell.getNZ(),
                          static_cast<std::size_t>(host_IJK[0]), static_cast<std::size_t>(host_IJK[1]), static_cast<std::size_t>(host_IJK[2]),
                            host_low_fatherIJK[0], host_low_fatherIJK[1], host_low_fatherIJK[2]);
            std::vector<int> host_cells_global_ref = getAllGlobalIndex(i_list, j_list, k_list);
            lgr_cell.set_hostnum(host_cells_global_ref);
            lgr_cell.init_children_host_cells();
        }
    }

    const std::vector<int>& EclipseGrid::getActiveMap() const {
        return m_active_to_global;
    }

    void EclipseGrid::assertLabelLGR(const std::string& label) const{
        if (std::find(all_lgr_labels.begin(), all_lgr_labels.end(), label) == all_lgr_labels.end()){
            throw std::invalid_argument("LGR label not found");
        }
    }


    void EclipseGrid::assertIndexLGR(std::size_t localIndex) const {
     if (std::find(lgr_active_index.begin(), lgr_active_index.end(), localIndex) != lgr_active_index.end()) {
        throw std::invalid_argument("input provided is an LGR refined cell");
      }
    }

    std::size_t EclipseGrid::getActiveIndexLGR(const std::string& label, std::size_t localIndex) const {
        assertLabelLGR(label);
        return activeIndexLGR(label, localIndex);
    }

    std::size_t EclipseGrid::getActiveIndexLGR(const std::string& label, std::size_t i, std::size_t j, std::size_t k) const {
        assertLabelLGR(label);
        return activeIndexLGR(label,i,j,k);
    }

    std::size_t EclipseGrid::activeIndexLGR(const std::string& label, std::size_t localIndex) const {
        if (lgr_label == label){
            std::size_t local_global_ind = getActiveIndex(localIndex);
            this->assertIndexLGR(local_global_ind);
            return lgr_level_active_map[local_global_ind] + lgr_global_counter;
        }
        else if (lgr_children_cells.empty()) {
            return 0;
        }
        else {
            return std::accumulate(lgr_children_cells.begin(), lgr_children_cells.end(), std::size_t{0},
                                   [&label,localIndex](std::size_t val, const auto& lgr_cell) {
                                       return val + lgr_cell.activeIndexLGR(label, localIndex);
                                   });
        }
    }


    std::size_t EclipseGrid::activeIndexLGR(const std::string& label, std::size_t i, std::size_t j, std::size_t k) const {
        if (lgr_label == label) {
            this->assertIJK(i,j,k);
            std::size_t local_global_ind = getActiveIndex(i,j,k);
            this->assertIndexLGR(local_global_ind);
            return lgr_level_active_map[local_global_ind] + lgr_global_counter;
        }
        else if (lgr_children_cells.empty()) {
            return 0;
        }
        else {
            return std::accumulate(lgr_children_cells.begin(), lgr_children_cells.end(), std::size_t{0},
                                   [&label,i,j,k](std::size_t val, const auto& lgr_cell) {
                                       return val + lgr_cell.activeIndexLGR(label, i, j, k);
                                   });
        }
    }


    std::size_t EclipseGrid::getTotalActiveLGR() const
    {
        std::size_t num_coarse_level =  getNumActive();
        std::size_t num_fine_cells = 0;
        std::size_t num_spanned_cells = 0;
        for (const auto&  fine_cells : lgr_children_cells) {
            num_fine_cells += fine_cells.getTotalActiveLGR();
            num_spanned_cells += fine_cells.get_father_global().size();

        }
        return num_coarse_level + num_fine_cells - num_spanned_cells;
    }


    void EclipseGrid::init_lgr_cells(const LgrCollection& lgr_input) {
        save_all_lgr_labels(lgr_input);
        create_lgr_cells_tree(lgr_input);
        //initialize LGRObjbect Indices
        initializeLGRObjectIndices(0);
        //parse fatherLGRObjbect Indices to children
        propagateParentIndicesToLGRChildren(0);
        // initialize the LGR tree indices for each refined cell.
        initializeLGRTreeIndices();
        // parse the reference indices to object in the global level.
        parseGlobalReferenceToChildren();
        // initialize the host cells for each LGR cell.
        // because the standard algorithm is based on topological information
        // it does not need for the refinement information to be parsed.
        init_children_host_cells();
        // initialize CPG refinement based parents COORD and ZCORN
        perform_refinement();

    }

    void EclipseGrid::perform_refinement(){
        for (auto& cell : lgr_children_cells) {
            cell.perform_refinement(getCOORD(), getZCORN(), getNXYZ());
        }
    }

    void EclipseGrid::propagateParentIndicesToLGRChildren(int index){
        lgr_level_father = index;
        for (auto& cell : lgr_children_cells) {
            cell.propagateParentIndicesToLGRChildren(lgr_level);
        }
    }

    int EclipseGrid::initializeLGRObjectIndices(int num){
        lgr_level = num;
        num++;
        std::accumulate(m_print_order_lgr_cells.begin(),
                        m_print_order_lgr_cells.end(), num,
                        [this](int n, const auto& it)
                        {
                            return lgr_children_cells[it].initializeLGRObjectIndices(n);
                        });
        return num;
    }

    void EclipseGrid::save_all_lgr_labels(const LgrCollection& lgr_input) {
        all_lgr_labels.reserve(lgr_input.size()+1);
        all_lgr_labels.push_back("GLOBAL");
        for (std::size_t index = 0; index < lgr_input.size(); index++) {
            all_lgr_labels.push_back(lgr_input.getLgr(index).NAME());
        }
    }

    void EclipseGrid::create_lgr_cells_tree(const LgrCollection& lgr_input) {
          auto IJK_global = [this](const auto& i_list, const auto& j_list, const auto& k_list){
            if (!(i_list.size() == j_list.size()) && (j_list.size() == k_list.size()) ){
                 throw std::invalid_argument("Sizes are not compatible.");
            }
            std::vector<std::size_t> global_ind_active(i_list.size());
            for (std::size_t index = 0; index < i_list.size(); index++) {
                global_ind_active[index] = this->getActiveIndex(i_list[index],j_list[index],k_list[index]);
            }
            return global_ind_active;
        };
         for (std::size_t index = 0; index < lgr_input.size(); index++) {
            const auto& lgr_cell = lgr_input.getLgr(index);
            if (this->lgr_label == lgr_cell.PARENT_NAME()){
                lgr_grid = true;
                // auto [i_list, j_list, k_list] = lgr_cell.parent_cellsIJK();
                auto [i_list, j_list, k_list] = VectorUtil::generate_cartesian_product(lgr_cell.I1(), lgr_cell.I2(),
                                                                                                                           lgr_cell.J1(), lgr_cell.J2(),
                                                                                                                           lgr_cell.K1(), lgr_cell.K2());

                auto father_lgr_index = IJK_global(i_list, j_list, k_list);

                std::array<int,3> lowIJK = {lgr_cell.I1(), lgr_cell.J1(),lgr_cell.K1()};
                std::array<int,3> upIJK  = {lgr_cell.I2(), lgr_cell.J2(),lgr_cell.K2()};

                lgr_children_cells.emplace_back(lgr_cell.NAME(), this->lgr_label,
                                                lgr_cell.NX(), lgr_cell.NY(), lgr_cell.NZ(), father_lgr_index,
                                                lowIJK,upIJK);

                lgr_children_cells.back().create_lgr_cells_tree(lgr_input);
            }
        }
        EclipseGridLGR::vec_size_t father_label_sorting(lgr_children_cells.size(),0);
        m_print_order_lgr_cells.resize(lgr_children_cells.size());
        std::iota(m_print_order_lgr_cells.begin(), m_print_order_lgr_cells.end(), 0); //
        std::ranges::transform(lgr_children_cells, father_label_sorting.begin(),
                               [](const auto& cell) { return cell.get_father_global()[0]; });

        std::ranges::sort(m_print_order_lgr_cells,
                          [&](std::size_t i1, std::size_t i2)
                          { return father_label_sorting[i1] < father_label_sorting[i2]; });

        std::ranges::sort(lgr_children_cells,
                          [](const EclipseGridLGR& a, const EclipseGridLGR& b)
                          { return a.get_father_global()[0] < b.get_father_global()[0]; });

        lgr_children_labels.reserve(lgr_children_cells.size());
        std::ranges::transform(lgr_children_cells, std::back_inserter(lgr_children_labels),
                               [](const auto& it) { return it.lgr_label; });
        lgr_active_index.resize(lgr_children_cells.size(),0);
    }

    void EclipseGrid::initializeLGRTreeIndices(){
        // initialize the LGR tree indices for each refined cell.
        auto set_map_scalar = [&](const auto& vec, const auto& value){
             num_lgr_children_cells[vec] = value;
        };
        auto set_vec_value = []( auto& vec, const auto& num_ref, const auto& data){
            for (std::size_t index = 0; index < num_ref.size(); index++) {
                vec[num_ref[index]] = data;
            }
        };
        std::vector<std::size_t> lgr_level_numbering_counting(getNumActive(),1);
        lgr_level_active_map.resize(getNumActive(),0);
        for (const auto& cell:lgr_children_cells) {
            set_map_scalar(cell.getFatherGlobalID(), cell.getTotalActiveLGR());
        }
        std::vector<std::size_t> bottom_lgr_cells;
        std::size_t index = 0;
        for (const auto& [key, value] : num_lgr_children_cells) {
            const std::size_t head_lgr_cell = key[0];
            lgr_level_numbering_counting[head_lgr_cell] = value;
            lgr_active_index[index] = head_lgr_cell;
            bottom_lgr_cells.insert(bottom_lgr_cells.end(), key.begin() + 1, key.end());
            set_vec_value(lgr_level_numbering_counting, bottom_lgr_cells, 0);
            index++;
        }
        std::partial_sum(lgr_level_numbering_counting.begin(), lgr_level_numbering_counting.end(),
                         lgr_level_active_map.begin());
        lgr_level_active_map.reserve(lgr_level_active_map.size()+1);
        lgr_level_active_map.insert(lgr_level_active_map.begin(),0);
        for (auto& lgr_cell : lgr_children_cells) {
            lgr_cell.initializeLGRTreeIndices();
        }
    }

    void EclipseGrid::parseGlobalReferenceToChildren(){
       for (std::size_t index = 0; index < lgr_children_cells.size(); index++)
        {
            lgr_children_cells[index].set_lgr_global_counter(lgr_level_active_map[lgr_active_index[index]] +
                                                             this->lgr_global_counter);
            lgr_children_cells[index].parseGlobalReferenceToChildren();
        }
    }


    void EclipseGrid::resetACTNUM() {
        std::size_t global_size = this->getCartesianSize();
        this->m_actnum.assign(global_size, 1);
        this->m_nactive = global_size;

        this->m_global_to_active.resize(global_size);
        std::iota(this->m_global_to_active.begin(), this->m_global_to_active.end(), 0);
        this->m_active_to_global = this->m_global_to_active;
        this->active_volume = std::nullopt;
    }

    void EclipseGrid::resetACTNUM(const int* actnum) {
        if (actnum == nullptr)
            this->resetACTNUM();
        else {
            auto global_size = this->getCartesianSize();
            this->m_global_to_active.clear();
            this->m_active_to_global.clear();
            this->m_actnum.resize(global_size);
            this->m_nactive = 0;

            for (std::size_t n = 0; n < global_size; n++) {
                this->m_actnum[n] = actnum[n];
                // numerical aquifer cells need to be active
                if (this->m_aquifer_cells.count(n) > 0) {
                    this->m_actnum[n] = 1;
                }
                if (this->m_actnum[n] > 0) {
                    this->m_global_to_active.push_back(this->m_nactive);
                    this->m_active_to_global.push_back(n);
                    this->m_nactive++;
                } else {
                    this->m_global_to_active.push_back(-1);

                }
            }
            this->active_volume = std::nullopt;
        }
    }


    void EclipseGrid::setMINPVV(const std::vector<double>& minpvv) {
        if (m_minpvMode == MinpvMode::Inactive || m_minpvMode == MinpvMode::EclSTD) {
            if (minpvv.size() != this->getCartesianSize()) {
                throw std::runtime_error("EclipseGrid::setMINPVV(): MINPVV vector size differs from logical cartesian size of grid.");
            }
            m_minpvVector = minpvv;
            m_minpvMode = MinpvMode::EclSTD;
        }
    }
    void EclipseGrid::resetACTNUM(const std::vector<int>& actnum) {
        if (actnum.size() != getCartesianSize())
            throw std::runtime_error("resetACTNUM(): actnum vector size differs from logical cartesian size of grid.");

        this->resetACTNUM(actnum.data());
    }

    ZcornMapper EclipseGrid::zcornMapper() const {
        return ZcornMapper( getNX() , getNY(), getNZ() );
    }

    void EclipseGrid::updateNumericalAquiferCells(const Deck& deck) {
        using AQUNUM = ParserKeywords::AQUNUM;
        if ( !deck.hasKeyword<AQUNUM>() ) {
            return;
        }
        const auto &aqunum_keywords = deck.getKeywordList<AQUNUM>();
        for (const auto &keyword : aqunum_keywords) {
            for (const auto &record : *keyword) {
                const std::size_t i = record.getItem<AQUNUM::I>().get<int>(0) - 1;
                const std::size_t j = record.getItem<AQUNUM::J>().get<int>(0) - 1;
                const std::size_t k = record.getItem<AQUNUM::K>().get<int>(0) - 1;
                const std::size_t global_index = this->getGlobalIndex(i, j, k);
                this->m_aquifer_cells.insert(global_index);

                if (! record.getItem<AQUNUM::DEPTH>().defaultApplied(0))
                    this->m_aquifer_cell_depths.insert_or_assign(global_index, record.getItem<AQUNUM::DEPTH>().getSIDouble(0));

                // Create map global_index -> (PVTNUM, SATNUM) to allow QC during FieldProps creation
                const int pvtnum = record.getItem<AQUNUM::PVT_TABLE_NUM>().defaultApplied(0) ? 0 : record.getItem<AQUNUM::PVT_TABLE_NUM>().get<int>(0);
                const int satnum = record.getItem<AQUNUM::SAT_TABLE_NUM>().defaultApplied(0) ? 0 : record.getItem<AQUNUM::SAT_TABLE_NUM>().get<int>(0);
                this->m_aquifer_cell_tabnums.insert_or_assign(global_index, std::array<int,2>{pvtnum, satnum});
            }
        }
    }

    const std::optional<MapAxes>& EclipseGrid::getMapAxes() const
    {
        return this->m_mapaxes;
    }


    ZcornMapper::ZcornMapper(std::size_t nx , std::size_t ny, std::size_t nz)
        : dims( {{nx,ny,nz}} ),
          stride( {{2 , 4*nx, 8*nx*ny}} ),
          cell_shift( {{0 , 1 , 2*nx , 2*nx + 1 , 4*nx*ny , 4*nx*ny + 1, 4*nx*ny + 2*nx , 4*nx*ny + 2*nx + 1 }})
    {
    }


    /* lower layer:   upper layer  (higher value of z - i.e. lower down in resrvoir).

         2---3           6---7
         |   |           |   |
         0---1           4---5
    */

    std::size_t ZcornMapper::index(std::size_t i, std::size_t j, std::size_t k, int c) const {
        if ((i >= dims[0]) || (j >= dims[1]) || (k >= dims[2]) || (c < 0) || (c >= 8))
            throw std::invalid_argument("Invalid cell argument");

        return i*stride[0] + j*stride[1] + k*stride[2] + cell_shift[c];
    }

    std::size_t ZcornMapper::size() const {
        return dims[0] * dims[1] * dims[2] * 8;
    }

    std::size_t ZcornMapper::index(std::size_t g, int c) const {
        int k = g / (dims[0] * dims[1]);
        g -= k * dims[0] * dims[1];

        int j = g / dims[0];
        g -= j * dims[0];

        int i = g;

        return index(i,j,k,c);
    }

    bool ZcornMapper::validZCORN( const std::vector<double>& zcorn) const {
        int sign = zcorn[ this->index(0,0,0,0) ] <= zcorn[this->index(0,0, this->dims[2] - 1,4)] ? 1 : -1;
        for (std::size_t j=0; j < this->dims[1]; j++)
            for (std::size_t i=0; i < this->dims[0]; i++)
                for (std::size_t c=0; c < 4; c++)
                    for (std::size_t k=0; k < this->dims[2]; k++) {
                        /* Between cells */
                        if (k > 0) {
                            std::size_t index1 = this->index(i,j,k-1,c+4);
                            std::size_t index2 = this->index(i,j,k,c);
                            if ((zcorn[index2] - zcorn[index1]) * sign < 0)
                                return false;
                        }

                        /* In cell */
                        {
                            std::size_t index1 = this->index(i,j,k,c);
                            std::size_t index2 = this->index(i,j,k,c+4);
                            if ((zcorn[index2] - zcorn[index1]) * sign < 0)
                                return false;
                        }
                    }

        return true;
    }


    std::size_t ZcornMapper::fixupZCORN( std::vector<double>& zcorn) {
        int sign = zcorn[ this->index(0,0,0,0) ] <= zcorn[this->index(0,0, this->dims[2] - 1,4)] ? 1 : -1;
        std::size_t cells_adjusted = 0;

        for (std::size_t k=0; k < this->dims[2]; k++)
            for (std::size_t j=0; j < this->dims[1]; j++)
                for (std::size_t i=0; i < this->dims[0]; i++)
                    for (std::size_t c=0; c < 4; c++) {
                        /* Cell to cell */
                        if (k > 0) {
                            std::size_t index1 = this->index(i,j,k-1,c+4);
                            std::size_t index2 = this->index(i,j,k,c);

                            if ((zcorn[index2] - zcorn[index1]) * sign < 0 ) {
                                zcorn[index2] = zcorn[index1];
                                cells_adjusted++;
                            }
                        }

                        /* Cell internal */
                        {
                            std::size_t index1 = this->index(i,j,k,c);
                            std::size_t index2 = this->index(i,j,k,c+4);

                            if ((zcorn[index2] - zcorn[index1]) * sign < 0 ) {
                                zcorn[index2] = zcorn[index1];
                                cells_adjusted++;
                            }
                        }
                    }
        return cells_adjusted;
    }

    CoordMapper::CoordMapper(std::size_t nx_, std::size_t ny_) :
        nx(nx_),
        ny(ny_)
    {
    }

    std::size_t CoordMapper::size() const {
        return (this->nx + 1) * (this->ny + 1) * 6;
    }


    std::size_t CoordMapper::index(std::size_t i, std::size_t j, std::size_t dim, std::size_t layer) const {
        if (i > this->nx)
            throw std::invalid_argument("Out of range");

        if (j > this->ny)
            throw std::invalid_argument("Out of range");

        if (dim > 2)
            throw std::invalid_argument("Out of range");

        if (layer > 1)
            throw std::invalid_argument("Out of range");

        return 6*( i + j*(this->nx + 1) ) +  layer * 3 + dim;
    }
}



namespace Opm {
    EclipseGridLGR::EclipseGridLGR(const std::string& self_label, const std::string& father_label_,
                                   std::size_t nx, std::size_t ny, std::size_t nz,
                                   const vec_size_t& father_lgr_index, const std::array<int,3>& low_fatherIJK_,
                                   const std::array<int,3>& up_fatherIJK_)
    : EclipseGrid(nx,ny,nz), father_label(father_label_), father_global(father_lgr_index),
                             low_fatherIJK(low_fatherIJK_), up_fatherIJK(up_fatherIJK_)
    {
        init_father_global();
        lgr_label= self_label;
    }

    std::vector<int> EclipseGridLGR::save_hostnum(void) const
    {
        std::vector<int> hostnum{this->m_hostnum};
        std::ranges::transform(hostnum, hostnum.begin(), [](int a) { return a + 1; });
        return hostnum;
    }


    std::vector<int> EclipseGridLGR::getLGRCell_global_father(const EclipseGrid& father_grid) const
    {
        const std::size_t n = getCartesianSize();
        std::vector<int> lgr_global_father(n);
        for (std::size_t i = 0; i < n; ++i) {
            lgr_global_father[i] = father_grid.getLGR_global_father(i, this->get_lgr_tag());
        }
        return lgr_global_father;
    }

    std::vector<double> EclipseGridLGR::getLGRCell_all_depth (const EclipseGrid& father_grid) const
    {
        const std::size_t n = getCartesianSize();
        const auto& lgr_label_ref = get_lgr_tag();
        std::vector<double> lgr_depths(n);

        const auto& local_lgr_grid = father_grid.getLGRCell(lgr_label_ref);

        for (std::size_t index = 0; index < n; ++index) {
            auto [i, j, k] = getIJK(index);

            lgr_depths[index] = local_lgr_grid.getCellDepth(i,j,k);
        }
        return lgr_depths;
    }


    void EclipseGridLGR::get_label_child_to_top_father(std::vector<std::reference_wrapper<const std::string>>& list) const
    {
        list.push_back(std::ref(lgr_label));
        if (father_label != "GLOBAL")
        {
            getLGRCell(father_label).get_label_child_to_top_father(list);
        }
    }

    void EclipseGridLGR::get_global_index_child_to_top_father(std::vector<std::size_t> & list,
                                                              std::size_t i, std::size_t j, std::size_t k) const
    {
        get_global_index_child_to_top_father(list, activeIndex(i,j,k));
    }

    void EclipseGridLGR::get_global_index_child_to_top_father(std::vector<std::size_t> & list, std::size_t global_ind) const
    {
        if (list.empty())
        {
            list.push_back(global_ind);
        }
        std::size_t father_id = get_hostnum(global_ind);
        list.push_back(father_id);
        if (father_label != "GLOBAL")
        {
            getLGRCell(father_label).get_global_index_child_to_top_father(list, father_id);
        }
    }

    std::optional<std::reference_wrapper<EclipseGridLGR>>
    EclipseGridLGR::get_child_LGR_cell(const std::string& lgr_tag) const
    {
        std::optional<std::reference_wrapper<EclipseGridLGR>> lgr_found = std::nullopt;
        // Otherwise, search recursively within children.
        if (lgr_tag == lgr_label)
        {
            lgr_found = std::ref(const_cast<EclipseGridLGR&>(*this));
        }
        else
        {
            for (auto &child : this->lgr_children_cells) {
                if ((child.get_lgr_tag() == lgr_tag) && (lgr_found == std::nullopt))
                {
                    lgr_found = std::ref(const_cast<EclipseGridLGR&>(child));
                }
                else
                {
                    return child.get_child_LGR_cell(lgr_tag);
                }
            }
        }
        return lgr_found;
    }

    void EclipseGridLGR::set_hostnum(const std::vector<int>& hostnum)
    {
        m_hostnum = hostnum;
    }

    void EclipseGridLGR::set_lgr_refinement(const std::string& lgr_tag, const std::vector<double>& coord, const std::vector<double>& zcorn)
    {
        if (lgr_tag == lgr_label)
        {
            this->set_lgr_refinement(coord, zcorn);
        } else
        {
            for (auto& lgr_cell : lgr_children_cells) {
                lgr_cell.set_lgr_refinement(lgr_tag, coord, zcorn);
            }
        }
    }
    void EclipseGridLGR::set_lgr_refinement(const std::vector<double>& coord, const std::vector<double>& zcorn)
    {
        m_coord = coord;
        m_zcorn = zcorn;
    }

    void EclipseGridLGR::init_father_global()
    {
        std::ranges::sort(father_global);
    }

    const EclipseGridLGR::vec_size_t& EclipseGridLGR::getFatherGlobalID() const
    {
        return father_global;
    }

    void EclipseGridLGR::save(Opm::EclIO::EclOutput& egridfile, const Opm::UnitSystem& units) const {
        save_core(egridfile, units);
        save_children(egridfile, units);
    }

    void EclipseGridLGR::perform_refinement(const std::vector<double>&  parent_coord,
                                             const std::vector<double>& parent_zcorn,
                                             const std::array<int,3>&   parent_nxyz)
    {
        m_coord = generate_refined_coord(parent_coord,  parent_nxyz);
        m_zcorn = generate_refined_zcorn(parent_coord, parent_zcorn, parent_nxyz);
        EclipseGrid::perform_refinement();
    }

    std::vector<double> EclipseGridLGR::generate_refined_zcorn(const std::vector<double>& parent_coord,
                                                               const std::vector<double>& parent_zcorn,
                                                               const std::array<int,3>&   parent_nxyz)
    {
        using CoordinateType = std::array<double,3>;
        using PillarType = std::array<std::array<double,3>, 2>;
        using ZCornType = double;

        struct Matrix {
            std::size_t rows, cols;
            std::vector<ZCornType> data;

            Matrix() : rows(0), cols(0), data() {}

            Matrix(std::size_t r, std::size_t c)
                : rows(r), cols(c), data(r * c, 0)
            {}

            ZCornType & operator()(std::size_t i, std::size_t j) {
                return data[i * cols + j];
            }

            const ZCornType& operator()(std::size_t i, std::size_t j) const {
                return data[i * cols + j];
            }
        };


        /**
         * Bilinear interpolation on a single layer of a quadrilateral,
         * selected from an 8-point array (bottom + top layers).
         *
         * Point order in each layer (0-3):
         *
         *     2 ---- 3    (top-left → top-right)
         *     |      |
         *     0 ---- 1    (bottom-left → bottom-right)
         *
         * points[0..3] -> bottom layer
         * points[4..7] -> top layer
         *
         * I,J: indices inside the grid cell
         * NI,NJ: maximum indices along each direction (size of the cell)
         * topLayer: false = use bottom layer, true = use top layer
         */

        auto bilinearInterpolation = [](const std::array<CoordinateType,8>& points,
                                                  int I, int J, int NI, int NJ,
                                                  bool topLayer) -> CoordinateType
        {
            int offset = topLayer ? 4 : 0;

            // Normalize indices to [0,1]
            double x = static_cast<double>(I) / NI;
            double y = static_cast<double>(J) / NJ;

            double w00 = (1 - x) * (1 - y);
            double w10 =      x  * (1 - y);
            double w01 = (1 - x) *      y;
            double w11 =      x  *      y;

            CoordinateType P;
            P[0] = w00*points[offset + 0][0] + w10*points[offset + 1][0] +
                   w01*points[offset + 2][0] + w11*points[offset + 3][0];

            P[1] = w00*points[offset + 0][1] + w10*points[offset + 1][1] +
                   w01*points[offset + 2][1] + w11*points[offset + 3][1];

            P[2] = w00*points[offset + 0][2] + w10*points[offset + 1][2] +
                   w01*points[offset + 2][2] + w11*points[offset + 3][2];

            return P;
        };

        auto generate_dest_ijk_refinement_intervals = [this](const std::vector<std::size_t>& i_elem,
                                                                       const std::vector<std::size_t>& j_elem,
                                                                       const std::vector<std::size_t>& k_elem)
        {

            std::size_t totalX = getNX();
            std::size_t totalY = getNY();
            std::size_t totalZ = getNZ();

            std::size_t min_i = *std::min_element(i_elem.begin(), i_elem.end());
            std::size_t max_i = *std::max_element(i_elem.begin(), i_elem.end());
            std::size_t min_j = *std::min_element(j_elem.begin(), j_elem.end());
            std::size_t max_j = *std::max_element(j_elem.begin(), j_elem.end());
            std::size_t min_k = *std::min_element(k_elem.begin(), k_elem.end());
            std::size_t max_k = *std::max_element(k_elem.begin(), k_elem.end());

            std::size_t num_el_i = max_i - min_i + 1;
            std::size_t num_el_j = max_j - min_j + 1;
            std::size_t num_el_k = max_k - min_k + 1;

            if (num_el_i == 0 || num_el_j == 0 || num_el_k == 0)
            {
                throw std::invalid_argument("Invalid number of elements.");
            }
            if (totalX % num_el_i != 0 || totalY % num_el_j != 0 || totalZ % num_el_k != 0) {
                // Either handle remainder properly or fail fast
                throw std::runtime_error("Refinement factors must exactly divide parent NX,NY and NZ");
            }

            auto dX = totalX/num_el_i;
            auto dY = totalY/num_el_j;
            auto dZ = totalZ/num_el_k;

            std::vector<std::size_t> refined_i_indices;
            std::vector<std::size_t> refined_j_indices;
            std::vector<std::size_t> refined_k_indices;

            refined_i_indices.resize(i_elem.size());
            refined_j_indices.resize(j_elem.size());
            refined_k_indices.resize(k_elem.size());

            std::ranges::transform(i_elem, refined_i_indices.begin(),
                                   [min_i, dX](std::size_t i)
                                   { return (i - min_i) * dX; });
            std::ranges::transform(j_elem, refined_j_indices.begin(),
                                   [min_j, dY](std::size_t j)
                                   { return (j - min_j) * dY; });
            std::ranges::transform(k_elem, refined_k_indices.begin(),
                                   [min_k, dZ](std::size_t k)
                                   { return (k - min_k) * dZ; });

            return std::make_tuple(refined_i_indices, refined_j_indices, refined_k_indices, dX, dY, dZ);
        };

        CoordMapper parent_coord_mapper(parent_nxyz[0], parent_nxyz[1]);
        ZcornMapper parent_zcorn_mapper(parent_nxyz[0], parent_nxyz[1], parent_nxyz[2]);

        CoordMapper local_coord_mapper(this->getNX(), this->getNY());
        ZcornMapper local_zcorn_mapper(this->getNX(), this->getNY(), this->getNZ());

        auto get_father_pillar = [&parent_coord_mapper, &parent_coord]( std::size_t i,
                                                                                 std::size_t j)
        {
            PillarType P;
            // Top point
            P[0][0] = parent_coord[ parent_coord_mapper.index(i, j, 0, 0) ]; // x_top
            P[0][1] = parent_coord[ parent_coord_mapper.index(i, j, 1, 0) ]; // y_top
            P[0][2] = parent_coord[ parent_coord_mapper.index(i, j, 2, 0) ]; // z_top
            // Bottom point
            P[1][0] = parent_coord[ parent_coord_mapper.index(i, j, 0, 1) ]; // x_bot
            P[1][1] = parent_coord[ parent_coord_mapper.index(i, j, 1, 1) ]; // y_bot
            P[1][2] = parent_coord[ parent_coord_mapper.index(i, j, 2, 1) ]; // z_bot
            return P;
        };

        auto get_local_pillar = [this, &local_coord_mapper]( std::size_t i, std::size_t j)
        {
            PillarType P;
            // Top point
            P[0][0] = m_coord[ local_coord_mapper.index(i, j, 0, 0) ]; // x_top
            P[0][1] = m_coord[ local_coord_mapper.index(i, j, 1, 0) ]; // y_top
            P[0][2] = m_coord[ local_coord_mapper.index(i, j, 2, 0) ]; // z_top
            // Bottom point
            P[1][0] = m_coord[ local_coord_mapper.index(i, j, 0, 1) ]; // x_bot
            P[1][1] = m_coord[ local_coord_mapper.index(i, j, 1, 1) ]; // y_bot
            P[1][2] = m_coord[ local_coord_mapper.index(i, j, 2, 1) ]; // z_bot
            return P;
        };

        auto get_zcorn_index = [&parent_zcorn_mapper]( std::size_t i,
                                                                 std::size_t j,
                                                                 std::size_t k)
        {
            std::array<std::size_t,8> father_zcorn_indices;
            for (int c = 0; c < 8; c++) {
                father_zcorn_indices[c] = parent_zcorn_mapper.index(i, j, k, c);
            }
            return father_zcorn_indices;
        };

        auto get_zcorn_value = [&parent_zcorn](std::array<std::size_t,8> indices)
        {
            std::array<double,8> zcorn_values;
            for (int idx = 0; idx < 8; idx++) {
                zcorn_values[idx] = parent_zcorn[ indices[idx] ];
            }
            return zcorn_values;
        };

        auto pillar_zcorn_to_coord = [](const PillarType& pillar, double zcorn_value) {
            double t = (zcorn_value - pillar[0][2]) / (pillar[1][2] - pillar[0][2]);
            double x = pillar[0][0] + t * (pillar[1][0] - pillar[0][0]);
            double y = pillar[0][1] + t * (pillar[1][1] - pillar[0][1]);
            return std::array<double, 3>{x, y, zcorn_value};
        };

        auto coord_pillar_to_zcorn = [](const PillarType& pillar, const std::array<double, 3>& coord) {
            double total_height = pillar[1][2] - pillar[0][2];
            if (total_height == 0.0) {
                return pillar[0][2]; // or pillar[1][2], they are the same
            }
            double t = (coord[2] - pillar[0][2]) / total_height;
            return pillar[0][2] + t * total_height;
        };

        auto find_coord_coorners = [&](const std::array<PillarType,4>& pillars,
                                                 const std::array<double,8>& father_zcorn_values)
        {
            auto coord_corners = std::array<std::array<double,3>,8>{};
            for (int c = 0; c < 8; c++) {
                coord_corners[c] = pillar_zcorn_to_coord(pillars[c % 4], father_zcorn_values[c]);
            }
            return coord_corners;
        };

        auto interpolate_inner_zcorn = [](std::vector<Matrix>& pillars_matrix)
        {
            std::size_t nz = pillars_matrix.size();
            for (std::size_t k = 1; k < nz - 1; k++) {
                double t = static_cast<double>(k) / (nz - 1);
                for (std::size_t j = 0; j < pillars_matrix[0].cols; j++) {
                    for (std::size_t i = 0; i < pillars_matrix[0].rows; i++) {
                        pillars_matrix[k](i,j) = (1 - t) * pillars_matrix[0](i,j) + t * pillars_matrix.back()(i,j);
                    }
                }
            }

        };


        const auto [NX, NY, NZ]  = getNXYZ();
        std::vector<double> refined_zcorn;
        refined_zcorn.resize(NX*NY*NZ*8);

        const auto [i_list, j_list, k_list ] =  VectorUtil::generate_cartesian_product(
            low_fatherIJK[0], up_fatherIJK[0],
            low_fatherIJK[1], up_fatherIJK[1],
            low_fatherIJK[2], up_fatherIJK[2]);

        const auto [dest_i_ref, dest_j_ref, dest_k_ref, dx , dy, dz] = generate_dest_ijk_refinement_intervals(i_list, j_list, k_list);


        for (std::size_t n = 0; n < i_list.size(); n++) {
            const std::size_t i_father = i_list[n];
            const std::size_t j_father = j_list[n];
            const std::size_t k_father = k_list[n];

            const std::size_t i_ref = dest_i_ref[n];
            const std::size_t j_ref = dest_j_ref[n];
            const std::size_t k_ref = dest_k_ref[n];

            std::vector<Matrix> local_zcorn_volumes;
            local_zcorn_volumes.resize(dz+1);
            for (std::size_t k = 0; k <= dz; k++) {
                local_zcorn_volumes[k] = Matrix(dx+1, dy+1);
            }

            std::array<PillarType,4> pillars;
            pillars[0] = get_father_pillar(i_father, j_father);
            pillars[1] = get_father_pillar(i_father+1, j_father);
            pillars[2] = get_father_pillar(i_father, j_father+1);
            pillars[3] = get_father_pillar(i_father+1, j_father+1);

            auto father_zcorn_indices = get_zcorn_index(i_father, j_father, k_father);
            auto father_zcorn_values = get_zcorn_value(father_zcorn_indices);
            auto coord_corners = find_coord_coorners(pillars, father_zcorn_values);

            // fill in the refined zcorn values of top and bottom layers
            for (std::size_t jj = 0; jj <= dy; jj++) {
                for (std::size_t ii = 0; ii <= dx; ii++) {
                    auto interpolated_coord_bottom = bilinearInterpolation(coord_corners, ii, jj, dx, dy, false);
                    auto interpolated_coord_top = bilinearInterpolation(coord_corners, ii, jj, dx, dy, true);
                    auto local_pillar = get_local_pillar(i_ref + ii, j_ref + jj);
                    local_zcorn_volumes[0](ii,jj) = coord_pillar_to_zcorn(local_pillar, interpolated_coord_bottom);
                    local_zcorn_volumes.back()(ii,jj) = coord_pillar_to_zcorn(local_pillar, interpolated_coord_top);
                }
            }

            // interpolate inner layers
            interpolate_inner_zcorn(local_zcorn_volumes);

            // assign to refined_zcorn
            for (std::size_t kk = 0; kk < dz; kk++) {
                for (std::size_t jj = 0; jj < dy; jj++) {
                    for (std::size_t ii = 0; ii < dx; ii++) {
                        std::size_t global_i = i_ref + ii;
                        std::size_t global_j = j_ref + jj;
                        std::size_t global_k = k_ref + kk;
                        std::array<double,8>  refined_zcorn_values = {local_zcorn_volumes[kk](ii,jj),
                                                                      local_zcorn_volumes[kk](ii+1,jj),
                                                                      local_zcorn_volumes[kk](ii,jj+1),
                                                                      local_zcorn_volumes[kk](ii+1,jj+1),
                                                                      local_zcorn_volumes[kk+1](ii,jj),
                                                                      local_zcorn_volumes[kk+1](ii+1,jj),
                                                                      local_zcorn_volumes[kk+1](ii,jj+1),
                                                                      local_zcorn_volumes[kk+1](ii+1,jj+1)};

                        for (std::size_t idx = 0; idx < 8; idx++) {
                            const std::size_t zcorn_idx = local_zcorn_mapper.index(global_i, global_j, global_k, idx);
                            refined_zcorn[zcorn_idx] = refined_zcorn_values[idx];
                        }

                    }
                }
            }
        }
        return refined_zcorn;
    }

    std::vector<double> EclipseGridLGR::generate_refined_coord(const std::vector<double>& coord_h,
                                                               const std::array<int,3>&   parent_nxyz)
    {

        constexpr std::array<std::array<int,2>,4> corner_offset = {{
            {0,0}, // 0
            {1,0}, // 1
            {1,1}, // 2
            {0,1}, // 3
        }};

        auto interpolatePillar = [](const double* p1,
                                              const double* p2,
                                              const double t,
                                              double* p)
        {
            for (std::size_t ix = 0; ix < std::size_t{6}; ++ix)
            {
                p[ix] = t * p2[ix] + (1.0 - t) * p1[ix];
            }
        };

        auto bilinear_interpolation = [](std::array<const double*, 4>& z,
                                                              double ti, double tj, auto* interpolated_coord)
        {
            for (std::size_t dim = 0; dim < std::size_t{6}; ++dim) {
                const double one_i = 1.0 - ti;
                const double one_j = 1.0 - tj;
                interpolated_coord[dim] =
                    z[0][dim] * one_i * one_j +
                    z[1][dim] * ti    * one_j +
                    z[2][dim] * ti    * tj    +
                    z[3][dim] * one_i * tj;
            }
        };

        const std::size_t nx = getNX();
        const std::size_t ny = getNY();

        const std::size_t NX = parent_nxyz[0];
        const std::size_t NY = parent_nxyz[1];

        const std::size_t Imin = low_fatherIJK[0];
        const std::size_t Imax = up_fatherIJK[0];

        const std::size_t Jmin = low_fatherIJK[1];
        const std::size_t Jmax = up_fatherIJK[1];

        const auto ch = CoordMapper { NX, NY }; // Host grid dimensions Mapper
        const auto cc = CoordMapper { nx, ny }; // Child/LGR grid dimensions Mapper

        const auto si = nx / (Imax - Imin + 1);
        const auto sj = ny / (Jmax - Jmin + 1);

        std::vector<double> coord_c;
        coord_c.resize(6 * (nx + 1) * (ny + 1));

        // Element loop: for each element in the parent grid, identified by (I,J) in between Imin and Imax and Jmin and Jmax
        for (auto J = Jmin; J <= Jmax; ++J) {
            for (auto I = Imin; I <= Imax; ++I) {
                // Retrieve Pillars Coordinates for the corners of the element (I,J) in the parent grid
                std::array<const double*, 4> pillars = {};
                for (std::size_t idx = 0; idx < corner_offset.size(); ++idx) {
                    const auto& offset = corner_offset[idx];
                    pillars[idx] = &coord_h[ch.index(I + offset[0], J + offset[1], 0, 0)];
                }
                // Loop through interal divisons inside the corners of element (I,J)
                // Looping from 0 to si or sj included means, looping through t=0 to t=1 included, thus covering
                // the whole area of the element (I,J) and including the corners.
                for (auto j = 0*sj; j < sj; ++j) {
                    const auto tj = static_cast<double>(j) / sj;
                    for (auto i = 0*si; i < si; ++i) {
                        const auto ti = static_cast<double>(i) / si;
                        const auto refined_index = &coord_c[cc.index((I - Imin)*si + i, (J - Jmin)*sj + j, 0, 0)];
                        bilinear_interpolation( pillars, ti, tj, refined_index);
                    }
                }
            }
        }

        // Interpolating the Pillars coords
        for (auto I = Imin; I <= Imax; ++I) {
                // Retrieve Pillars Coordinates for the corners of the element (I,J) in the parent grid
                std::array<const double*, 2> pillars = {};
                std::size_t index = 0;
                // Retrieve TOP Pillars Coordinates of Host Cell for the top row of elements in the LGR
                for (const auto& offset : {corner_offset[3], corner_offset[2]}) {
                    pillars[index++] = &coord_h[ch.index(I + offset[0], Jmax + offset[1], 0, 0)];
                }
                for (auto i = 0*si; i < si; ++i) {
                    const auto ti = static_cast<double>(i) / si;
                    const auto refined_index = &coord_c[cc.index((I - Imin)*si + i, ny , 0, 0)];
                    interpolatePillar(pillars[0], pillars[1], ti, refined_index);
                }
        }

        for (auto J = Jmin; J <= Jmax; ++J) {
            // Retrieve Pillars Coordinates for the corners of the element (I,J) in the parent grid
            std::array<const double*, 2> pillars = {};
            std::size_t index = 0;
            // Retrieve RIGHT Pillars Coordinates of Host Cell between for Pillar
            //  {Imax+1,J} to 2 {Imax+1,J+1} . I.e. RIGHT Pillars of the rightmost column of elements in the LGR
            for (const auto& offset : {corner_offset[1], corner_offset[2]}) {
                pillars[index++] = &coord_h[ch.index(Imax + offset[0], J + offset[1], 0, 0)];
            }
            for (auto j = 0*sj; j < sj; ++j) {
                const auto tj = static_cast<double>(j) / sj;
                const auto refined_index = &coord_c[cc.index(nx, (J - Jmin)*sj + j, 0, 0)];
                interpolatePillar(pillars[0], pillars[1], tj, refined_index);
            }
        }

        // adding the coordinate of the top right corner of the LGR, which is the only one not covered by the previous loops
        {
            // Retrieve TOP Right Pillars Coordinates of Host Cell between for Pillar
            const double* host_top_right_pillar = &coord_h[ch.index(Imax + 1, Jmax + 1, 0, 0)];
            double* refined_index = &coord_c[cc.index(nx, ny, 0, 0)];
            for (std::size_t dim = 0; dim < 6; ++dim) {
                refined_index[dim] = host_top_right_pillar[dim];
            }
        }

        return coord_c;
    }

    void EclipseGridLGR::save_core(Opm::EclIO::EclOutput& egridfile, const Opm::UnitSystem& units) const{
        const auto lgr_name_label = std::vector{ Opm::EclIO::PaddedOutputString<8>{ lgr_label }};
        egridfile.write("LGR",lgr_name_label);
        //std::vector<Opm::EclIO::PaddedOutputString<8>> lgr_father_name_label;
        std::vector<std::string> lgr_father_name_label;
        if (lgr_level_father == 0){
            lgr_father_name_label.push_back("");
        }
        else {
            lgr_father_name_label.push_back(father_label);
        }
        egridfile.write("LGRPARNT", lgr_father_name_label);

        constexpr auto length = ::Opm::UnitSystem::measure::length;

        const std::array<int, 3> dims = getNXYZ();

        // Preparing vectors to be saved

        // create coord vector of floats with input units, converted from SI
        std::vector<float> coord_f;
        coord_f.resize(m_coord.size());

        auto convert_length = [&units](const double x) { return static_cast<float>(units.from_si(length, x)); };

        if (m_input_coord.has_value()) {
            std::ranges::transform(m_input_coord.value(), coord_f.begin(), convert_length);
        } else {
            std::ranges::transform(m_coord, coord_f.begin(), convert_length);
        }

        // create zcorn vector of floats with input units, converted from SI
        std::vector<float> zcorn_f;
        zcorn_f.resize(m_zcorn.size());

        if (m_input_zcorn.has_value()) {
            std::ranges::transform(m_input_zcorn.value(), zcorn_f.begin(), convert_length);
        } else {
            std::ranges::transform(m_zcorn, zcorn_f.begin(), convert_length);
        }

        m_input_coord.reset();
        m_input_zcorn.reset();

        // corner point grid

        std::vector<int> gridhead(100,0);
        // GLOBAL and LGR Gridhead
        gridhead[0] = 1;                    // corner point grid
        gridhead[1] = dims[0];              // nI
        gridhead[2] = dims[1];              // nJ
        gridhead[3] = dims[2];              // nK
        gridhead[4] = lgr_level;            // LGR index

        // LGR Exclusive Gridhead Flags
        gridhead[24] = 1;                   // number of reservoirs
        gridhead[25] = 1;                   // number of coordinate line seg
        gridhead[26] = 0;                   // NTHETA =0 non-radial
        gridhead[27] = low_fatherIJK[0] + 1;// Lower I-index-host
        gridhead[28] = low_fatherIJK[1] + 1;// Lower J-index-host
        gridhead[29] = low_fatherIJK[2] + 1;// Lower K-index-host
        gridhead[30] = up_fatherIJK[0] + 1; // Upper I-index-host
        gridhead[31] = up_fatherIJK[1] + 1; // Upper J-index-host
        gridhead[32] = up_fatherIJK[2] + 1; // Upper K-index-host


        std::vector<int> endgrid = {};

        // Writing vectors to egrid file

        egridfile.write("GRIDHEAD", gridhead);

        egridfile.write("COORD", coord_f);
        egridfile.write("ZCORN", zcorn_f);

        egridfile.write("ACTNUM", m_actnum);
        egridfile.write("HOSTNUM", save_hostnum());
        egridfile.write("ENDGRID", endgrid);
        egridfile.write("ENDLGR", endgrid);


    }
}
