/*
  Copyright 2015 Statoil ASA.

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

#include <opm/input/eclipse/EclipseState/IOConfig/IOConfig.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/D.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/F.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/I.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/M.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/N.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/U.hpp>

#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

namespace {

    const char* default_dir = ".";

    bool write_all_trans_multipliers(const Opm::RUNSPECSection& runspec)
    {
        using Kw = Opm::ParserKeywords::GRIDOPTS;

        if (! runspec.hasKeyword<Kw>()) {
            return false;
        }

        return runspec.get<Kw>().back()
            .getRecord(0)
            .getItem<Kw::TRANMULT>()
            .getTrimmedString(0) == "YES";
    }

    bool write_egrid_file(const Opm::GRIDSection& grid)
    {
        if (grid.hasKeyword<Opm::ParserKeywords::NOGGF>()) {
            return false;
        }

        if (! grid.hasKeyword<Opm::ParserKeywords::GRIDFILE>()) {
            return true;
        }

        const auto& rec = grid.get<Opm::ParserKeywords::GRIDFILE>()
            .back().getRecord(0);

        {
            const auto& grid_item = rec.getItem(0);

            if (grid_item.get<int>(0) != 0) {
                std::cerr << "IOConfig: Reading GRIDFILE keyword from GRID section: "
                          << "Output of GRID file is not supported. "
                          << "Supported format: EGRID\n";

                // It was asked for GRID file - that output is not
                // supported, but we will output EGRID file;
                // irrespective of whether that was actually
                // requested.
                return true;
            }
        }

        {
            const auto& egrid_item = rec.getItem(1);

            return egrid_item.get<int>(0) == 1;
        }
    }

    std::string parseFluxType(const Opm::GRIDSection& grid)
    {
        if (!grid.hasKeyword<Opm::ParserKeywords::FLUXTYPE>()) {
            return "FLUX";
        }

        auto mode = grid.get<Opm::ParserKeywords::FLUXTYPE>()
            .back()
            .getRecord(0)
            .getItem<Opm::ParserKeywords::FLUXTYPE::BC_TYPE>()
            .getTrimmedString(0);

        std::ranges::transform(mode,
                               mode.begin(),
                               [](const unsigned char c)
                               {
                                   return static_cast<char>(std::toupper(c));
                               });

        if (mode == "FLUX" || mode == "PRESSURE" || mode == "BOTH") {
            return mode;
        }

        Opm::OpmLog::warning("Unrecognized FLUXTYPE BC_TYPE value '" + mode
                             + "', defaulting to FLUX output mode");
        return "FLUX";
    }

    bool parseUseFlux(const Opm::GRIDSection& grid)
    {
        return grid.hasKeyword<Opm::ParserKeywords::USEFLUX>();
    }

    // FLUXNUM is an ordinary region array that other keywords, MULTREGT
    // among them, may use for their own purposes. Only DUMPFLUX asks for
    // the sector boundary to be written, and a run that both reads one and
    // writes one is not supported.
    bool parseDumpFlux(const Opm::GRIDSection& grid)
    {
        return grid.hasKeyword<Opm::ParserKeywords::DUMPFLUX>()
            && !grid.hasKeyword<Opm::ParserKeywords::USEFLUX>();
    }

    std::string parseUseFluxBaseName(const Opm::GRIDSection& grid)
    {
        if (!grid.hasKeyword<Opm::ParserKeywords::USEFLUX>()) {
            return {};
        }

        const auto& useflux = grid.get<Opm::ParserKeywords::USEFLUX>().back();
        if (useflux.empty()) {
            return {};
        }

        return useflux
            .getRecord(0)
            .getItem<Opm::ParserKeywords::USEFLUX::FLUX_BASENAME>()
            .getTrimmedString(0);
    }

    std::vector<int> parseUseFluxRegions(const Opm::GRIDSection& grid)
    {
        if (!grid.hasKeyword<Opm::ParserKeywords::FLUXREG>()) {
            return {};
        }

        const auto& item = grid.get<Opm::ParserKeywords::FLUXREG>()
            .back()
            .getRecord(0)
            .getItem<Opm::ParserKeywords::FLUXREG::REGIONS>();

        return item.getData<int>();
    }

    bool normalize_case(std::string& s)
    {
        int upper_count = 0;
        int lower_count = 0;

        for (const auto& c : s) {
            upper_count += std::isupper(c);
            lower_count += std::islower(c);
        }

        if (upper_count * lower_count == 0) {
            return false;
        }

        std::ranges::transform(s, s.begin(),
                               [](const auto& c) { return std::toupper(c); });

        return true;
    }

    std::string basename(const std::string& path)
    {
        return std::filesystem::path(path).stem().string();
    }

    std::string outputdir(const std::string& path)
    {
        const auto dir = std::filesystem::path(path).parent_path().string();

        if (dir.empty()) {
            return default_dir;
        }

        return dir;
    }

} // Anonymous namespace

namespace Opm {

    IOConfig::IOConfig(const Deck& deck)
        : IOConfig(GRIDSection(deck),
                   RUNSPECSection(deck),
                   deck.hasKeyword<ParserKeywords::NOSIM>(),
                   deck.getDataFile())
    {}

    IOConfig::IOConfig(const std::string& input_path)
        : m_deck_filename(input_path)
        , m_output_dir(outputdir(input_path))
    {
        this->setBaseName(basename(input_path));
    }

    IOConfig IOConfig::serializationTestObject()
    {
        IOConfig result;

        result.m_deck_filename = "test1";
        result.m_output_dir = "test2";

        result.m_write_INIT_file = true;
        result.m_write_EGRID_file = false;
        result.m_FMTIN = true;
        result.m_FMTOUT = true;

        result.m_nosim = true;
        result.m_write_all_multminus = true;

        result.m_base_name = "test3";

        result.m_UNIFIN = true;
        result.m_UNIFOUT = true;
        result.m_use_flux = true;
        result.m_dump_flux = true;
        result.m_use_flux_base_name = "test4";
        result.m_use_flux_regions = {2, 5};
        result.m_flux_type = "BOTH";

        result.m_output_enabled = false;
        result.ecl_compatible_rst = false;

        return result;
    }

    IOConfig::IOConfig(const GRIDSection&    grid,
                       const RUNSPECSection& runspec,
                       const bool            nosim,
                       const std::string&    input_path)
        : m_deck_filename      (input_path)
        , m_output_dir         (outputdir(input_path))
        , m_write_INIT_file    (grid.hasKeyword<ParserKeywords::INIT>())
        , m_write_EGRID_file   (write_egrid_file(grid))
        , m_FMTIN              (runspec.hasKeyword<ParserKeywords::FMTIN>())
        , m_FMTOUT             (runspec.hasKeyword<ParserKeywords::FMTOUT>())
        , m_nosim              (nosim)
        , m_write_all_multminus(write_all_trans_multipliers(runspec))
        , m_use_flux           (parseUseFlux(grid))
        , m_dump_flux          (parseDumpFlux(grid))
        , m_use_flux_base_name (parseUseFluxBaseName(grid))
        , m_use_flux_regions   (parseUseFluxRegions(grid))
        , m_flux_type          (parseFluxType(grid))
    {
        this->setBaseName(basename(input_path));

        // Loop keywords in RUNSPEC to determine unified vs. separate
        // input/output file type flags because the last UNIF*/MULT* keyword
        // "wins".
        for (const auto& kw : runspec) {
            if (kw.name() == ParserKeywords::UNIFOUT::keywordName) {
                this->m_UNIFOUT = true;
            }
            else if (kw.name() == ParserKeywords::UNIFIN::keywordName) {
                this->m_UNIFIN = true;
            }
            else if (kw.name() == ParserKeywords::MULTOUT::keywordName) {
                this->m_UNIFOUT = false;
            }
            else if (kw.name() == ParserKeywords::MULTIN::keywordName) {
                this->m_UNIFIN = false;
            }
        }
    }

    bool IOConfig::getWriteEGRIDFile() const
    {
        return m_write_EGRID_file;
    }

    bool IOConfig::getWriteINITFile() const
    {
        return m_write_INIT_file;
    }

    bool IOConfig::getEclCompatibleRST() const
    {
        return this->ecl_compatible_rst;
    }

    void IOConfig::setEclCompatibleRST(bool ecl_rst)
    {
        this->ecl_compatible_rst = ecl_rst;
    }

    void IOConfig::overrideNOSIM(bool nosim)
    {
        m_nosim = nosim;
    }

    bool IOConfig::getUNIFIN() const
    {
        return m_UNIFIN;
    }

    bool IOConfig::getUNIFOUT() const
    {
        return m_UNIFOUT;
    }

    void IOConfig::consistentFileFlags()
    {
        this->m_UNIFIN = getUNIFOUT();
        this->m_FMTIN = getFMTOUT();
    }

    bool IOConfig::getFMTIN() const
    {
        return m_FMTIN;
    }

    bool IOConfig::getFMTOUT() const
    {
        return m_FMTOUT;
    }

    bool IOConfig::getUseFlux() const
    {
        return this->m_use_flux;
    }

    bool IOConfig::getDumpFlux() const
    {
        return this->m_dump_flux;
    }

    const std::string& IOConfig::getUseFluxBaseName() const
    {
        return this->m_use_flux_base_name;
    }

    std::string IOConfig::getUseFluxInputBaseName() const
    {
        return this->m_use_flux_base_name.empty() ? this->getBaseName() : this->m_use_flux_base_name;
    }

    const std::vector<int>& IOConfig::getUseFluxRegions() const
    {
        return this->m_use_flux_regions;
    }

    const std::string& IOConfig::getFluxType() const
    {
        return this->m_flux_type;
    }

    std::string IOConfig::getRestartFileName(const std::string& restart_base,
                                             const int          report_step,
                                             const bool         output) const
    {
        const bool unified  = output ? getUNIFOUT() : getUNIFIN();
        const bool fmt_file = output ? getFMTOUT()  : getFMTIN();

        auto ext = std::string{};
        if (unified) {
            ext = fmt_file ? "FUNRST" : "UNRST";
        }
        else {
            std::ostringstream os;

            const char* fmt_prefix   = "FGH";
            const char* unfmt_prefix = "XYZ";

            const int cycle = 10 * 1000;
            const int p_ix  = report_step / cycle;
            const int n     = report_step % cycle;

            os << (fmt_file ? fmt_prefix[p_ix] : unfmt_prefix[p_ix])
               << std::setw(4) << std::setfill('0') << n;

            ext = os.str();
        }

        return restart_base + '.' + ext;
    }

    bool IOConfig::getOutputEnabled() const
    {
        return m_output_enabled;
    }

    void IOConfig::setOutputEnabled(bool enabled)
    {
        m_output_enabled = enabled;
    }

    const std::string& IOConfig::getOutputDir() const
    {
        return m_output_dir;
    }

    std::string IOConfig::getInputDir() const
    {
        const auto path = std::filesystem::path {this->m_deck_filename};

        return path.has_parent_path()
            ? path.parent_path().generic_string()
            : std::filesystem::current_path().generic_string();
    }

    void IOConfig::setOutputDir(const std::string& outputDir)
    {
        m_output_dir = outputDir;
    }

    const std::string& IOConfig::getBaseName() const
    {
        return m_base_name;
    }

    void IOConfig::setBaseName(const std::string& baseName)
    {
        this->m_base_name = baseName;

        if (normalize_case(m_base_name)) {
            OpmLog::warning("The ALL CAPS case: " + m_base_name +
                            " will be used when writing output "
                            "files from this simulation.");
        }
    }

    std::string IOConfig::fullBasePath() const
    {
        namespace fs = std::filesystem;

        fs::path dir( m_output_dir );
        fs::path base( m_base_name );
        fs::path full_path = dir.make_preferred() / base.make_preferred();

        return full_path.string();
    }

    bool IOConfig::initOnly() const
    {
        return m_nosim;
    }

    bool IOConfig::operator==(const IOConfig& data) const
    {
        return (this->getWriteINITFile() == data.getWriteINITFile())
            && (this->getWriteEGRIDFile() == data.getWriteEGRIDFile())
            && (this->getUNIFIN() == data.getUNIFIN())
            && (this->getUNIFOUT() == data.getUNIFOUT())
            && (this->getFMTIN() == data.getFMTIN())
            && (this->getFMTOUT() == data.getFMTOUT())
            && (this->writeAllTransMultipliers() == data.writeAllTransMultipliers())
            && (this->m_deck_filename == data.m_deck_filename)
            && (this->getOutputEnabled() == data.getOutputEnabled())
            && (this->getOutputDir() == data.getOutputDir())
            && (this->initOnly() == data.initOnly())
            && (this->getBaseName() == data.getBaseName())
            && (this->getEclCompatibleRST() == data.getEclCompatibleRST())
            && (this->getUseFlux() == data.getUseFlux())
            && (this->getDumpFlux() == data.getDumpFlux())
            && (this->getUseFluxBaseName() == data.getUseFluxBaseName())
            && (this->getUseFluxRegions() == data.getUseFluxRegions())
            && (this->getFluxType() == data.getFluxType())
            ;
    }

    bool IOConfig::rst_cmp(const IOConfig& full_config,
                           const IOConfig& rst_config)
    {
        return (full_config.getWriteINITFile() == rst_config.getWriteINITFile())
            && (full_config.getWriteEGRIDFile() == rst_config.getWriteEGRIDFile())
            && (full_config.getUNIFIN() == rst_config.getUNIFIN())
            && (full_config.getUNIFOUT() == rst_config.getUNIFOUT())
            && (full_config.getFMTIN() == rst_config.getFMTIN())
            && (full_config.getFMTOUT() == rst_config.getFMTOUT())
            ;
    }

    // ------------------------------------------------------------------------
    //
    // Here at the bottom are some forwarding proxy methods which just
    // forward to the appropriate RestartConfig method. They are retained
    // here as a temporary convenience method to prevent downstream
    // breakage.
    //
    // Currently the EclipseState object can return a mutable IOConfig
    // object, which application code can alter to override settings from
    // the deck - this is quite ugly. When the API is reworked to remove the
    // ability modify IOConfig objects we should also remove these
    // forwarding methods.

} // namespace Opm
