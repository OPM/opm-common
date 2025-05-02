/*
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
#include "config.h"

#include <opm/output/eclipse/EclipseIO.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

#include <opm/input/eclipse/Schedule/RPTConfig.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <opm/input/eclipse/Units/Dimension.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <opm/output/eclipse/AggregateAquiferData.hpp>
#include <opm/output/eclipse/RestartIO.hpp>
#include <opm/output/eclipse/RestartValue.hpp>
#include <opm/output/eclipse/Summary.hpp>
#include <opm/output/eclipse/WriteInit.hpp>
#include <opm/output/eclipse/WriteRFT.hpp>

#include <opm/io/eclipse/ESmry.hpp>
#include <opm/io/eclipse/OutputStream.hpp>

#include <opm/common/utility/String.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <limits>
#include <map>
#include <memory>     // unique_ptr
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>    // move
#include <vector>

namespace {

void ensure_directory_exists(const std::filesystem::path& odir)
{
    namespace fs = std::filesystem;

    if (fs::exists(odir) && !fs::is_directory(odir)) {
        throw std::runtime_error {
            "Filesystem element '" + odir.generic_string()
            + "' already exists but is not a directory"
        };
    }

    std::error_code ec{};
    if (! fs::exists(odir)) {
        fs::create_directories(odir, ec);
    }

    if (ec) {
        std::ostringstream msg;

        msg << "Failed to create output directory '"
            << odir.generic_string()
            << "\nSystem reports: " << ec << '\n';

        throw std::runtime_error { msg.str() };
    }
}

} // Anonymous namespace

class Opm::EclipseIO::Impl
{
public:
    Impl(const EclipseState&  eclipseState,
         EclipseGrid          grid,
         const Schedule&      schedule,
         const SummaryConfig& summryConfig,
         const std::string&   baseName,
         const bool           writeEsmry);

    bool outputEnabled() const { return this->output_enabled_; }

    std::pair<bool, bool>
    wantRFTOutput(const int report_step, const bool isSubstep) const;

    bool wantSummaryOutput(const int          report_step,
                           const bool         isSubstep,
                           const double       secs_elapsed,
                           std::optional<int> time_step) const;

    bool wantRestartOutput(const int          report_step,
                           const bool         isSubstep,
                           std::optional<int> time_step) const;

    bool isFinalStep(const int report_step) const;

    const std::string& outputDir() const { return this->outputDir_; }
    const out::Summary& summary() const { return this->summary_; }
    const SummaryConfig& summaryConfig() const { return this->summaryConfig_; }

    RestartValue loadRestart(const std::vector<RestartKey>& solution_keys,
                             const std::vector<RestartKey>& extra_keys,
                             Action::State&                 action_state,
                             SummaryState&                  summary_state) const;

    data::Solution loadRestartSolution(const std::vector<RestartKey>& solution_keys,
                                       const int                      report_step) const;

    void writeInitial(data::Solution                          simProps,
                      std::map<std::string, std::vector<int>> int_data,
                      const std::vector<NNCdata>&             nnc) const;

    void writeSummaryFile(const SummaryState&      st,
                          const int                report_step,
                          const std::optional<int> time_step,
                          const double             secs_elapsed,
                          const bool               isSubstep);

    void writeRestartFile(const Action::State& action_state,
                          const WellTestState& wtest_state,
                          const SummaryState&  st,
                          const UDQState&      udq_state,
                          const int            report_step,
                          std::optional<int>   time_step,
                          const double         secs_elapsed,
                          const bool           write_double,
                          RestartValue&&       value);

    void writeRunSummary() const;

    void writeRftFile(const double       secs_elapsed,
                      const int          report_step,
                      const bool         haveExistingRFT,
                      const data::Wells& wellSol) const;

private:
    std::reference_wrapper<const EclipseState> es_;
    std::reference_wrapper<const Schedule> schedule_;
    EclipseGrid grid_;

    std::string outputDir_;
    std::string baseName_;
    SummaryConfig summaryConfig_;
    out::Summary summary_;
    bool output_enabled_{false};

    std::optional<RestartIO::Helpers::AggregateAquiferData> aquiferData_{std::nullopt};

    mutable bool sumthin_active_{false};
    mutable bool sumthin_triggered_{false};

    double last_sumthin_output_{std::numeric_limits<double>::lowest()};

    void writeInitFile(data::Solution                          simProps,
                       std::map<std::string, std::vector<int>> int_data,
                       const std::vector<NNCdata>&             nnc) const;

    void writeEGridFile(const std::vector<NNCdata>& nnc) const;

    void recordSummaryOutput(const double secs_elapsed);

    int reportIndex(const int report_step, const std::optional<int> time_step) const;

    bool checkAndRecordIfSumthinTriggered(const int report_step,
                                          const double secs_elapsed) const;

    bool summaryAtRptOnly(const int report_step) const;
};

Opm::EclipseIO::Impl::Impl(const EclipseState&  eclipseState,
                           EclipseGrid          grid,
                           const Schedule&      schedule,
                           const SummaryConfig& summaryConfig,
                           const std::string&   base_name,
                           const bool           writeEsmry)
    : es_            (std::cref(eclipseState))
    , schedule_      (std::cref(schedule))
    , grid_          (std::move(grid))
    , outputDir_     (eclipseState.getIOConfig().getOutputDir())
    , baseName_      (uppercase(eclipseState.getIOConfig().getBaseName()))
    , summaryConfig_ (summaryConfig)
    , summary_       (summaryConfig_, es_, grid_, schedule, base_name, writeEsmry)
    , output_enabled_(eclipseState.getIOConfig().getOutputEnabled())
{
    if (const auto& aqConfig = this->es_.get().aquifer();
        aqConfig.connections().active() || aqConfig.hasNumericalAquifer())
    {
        this->aquiferData_
            .emplace(RestartIO::inferAquiferDimensions(this->es_),
                     aqConfig, this->grid_);
    }
}

std::pair<bool, bool>
Opm::EclipseIO::Impl::wantRFTOutput(const int  report_step,
                                    const bool isSubstep) const
{
    if (isSubstep) {
        return { false, false };
    }

    const auto first_rft = this->schedule_.get().first_RFT();
    if (! first_rft.has_value()) {
        return { false, false };
    }

    const auto first_rft_out = first_rft.value();
    const auto step = static_cast<std::size_t>(report_step);

    return {
        step >= first_rft_out,  // wantRFT
        step >  first_rft_out   // haveExistingRFT
    };
}

bool Opm::EclipseIO::Impl::isFinalStep(const int report_step) const
{
    return report_step == static_cast<int>(this->schedule_.get().size()) - 1;
}

bool Opm::EclipseIO::Impl::wantSummaryOutput(const int          report_step,
                                             const bool         isSubstep,
                                             const double       secs_elapsed,
                                             std::optional<int> time_step) const
{
    if (time_step.has_value()) {
        return true;
    }

    if (report_step <= 0) {
        return false;
    }

    // Check this condition first because the end of a SUMTHIN interval
    // might coincide with a report step.  In that case we also need to
    // reset the interval starting point even if the primary reason for
    // generating summary output is the report step.
    this->checkAndRecordIfSumthinTriggered(report_step, secs_elapsed);

    return !isSubstep
        || (!this->summaryAtRptOnly(report_step)
            && (!this->sumthin_active_ || this->sumthin_triggered_));
}

bool Opm::EclipseIO::Impl::wantRestartOutput(const int          report_step,
                                             const bool         isSubstep,
                                             std::optional<int> time_step) const
{
    return (time_step.has_value() && (*time_step > 0))
        || (!isSubstep && this->schedule_.get().write_rst_file(report_step));
}

Opm::RestartValue
Opm::EclipseIO::Impl::loadRestart(const std::vector<RestartKey>& solution_keys,
                                  const std::vector<RestartKey>& extra_keys,
                                  Action::State&                 action_state,
                                  SummaryState&                  summary_state) const
{
    const auto& initConfig = this->es_.get().getInitConfig();

    const auto report_step = initConfig.getRestartStep();
    const auto filename    = this->es_.get().cfg().io()
        .getRestartFileName(initConfig.getRestartRootName(),
                            report_step,
                            /* for file writing output = */ false);

    return RestartIO::load(filename,
                           report_step,
                           action_state,
                           summary_state,
                           solution_keys,
                           this->es_,
                           this->grid_,
                           this->schedule_,
                           extra_keys);
}

Opm::data::Solution
Opm::EclipseIO::Impl::loadRestartSolution(const std::vector<RestartKey>& solution_keys,
                                          const int                      report_step) const
{
    const auto& initConfig  = this->es_.get().getInitConfig();
    const auto  filename    = this->es_.get().getIOConfig()
        .getRestartFileName(initConfig.getRestartRootName(), report_step, false);

    return RestartIO::load_solution_only(filename, report_step, solution_keys,
                                         this->es_, this->grid_);
}

void Opm::EclipseIO::Impl::writeInitial(data::Solution                          simProps,
                                        std::map<std::string, std::vector<int>> int_data,
                                        const std::vector<NNCdata>&             nnc) const
{
    if (this->es_.get().cfg().io().getWriteINITFile()) {
        this->writeInitFile(std::move(simProps), std::move(int_data), nnc);
    }

    if (this->es_.get().cfg().io().getWriteEGRIDFile()) {
        this->writeEGridFile(nnc);
    }
}

void Opm::EclipseIO::Impl::writeSummaryFile(const SummaryState&      st,
                                            const int                report_step,
                                            const std::optional<int> time_step,
                                            const double             secs_elapsed,
                                            const bool               isSubstep)
{
    this->summary_.add_timestep(st, this->reportIndex(report_step, time_step),
                                !time_step.has_value() || isSubstep);

    const auto is_final_summary =
        this->isFinalStep(report_step) && !isSubstep;

    this->summary_.write(is_final_summary);

    this->recordSummaryOutput(secs_elapsed);
}

void Opm::EclipseIO::Impl::writeRestartFile(const Action::State& action_state,
                                            const WellTestState& wtest_state,
                                            const SummaryState&  st,
                                            const UDQState&      udq_state,
                                            const int            report_step,
                                            std::optional<int>   time_step,
                                            const double         secs_elapsed,
                                            const bool           write_double,
                                            RestartValue&&       value)
{
    EclIO::OutputStream::Restart rstFile {
        EclIO::OutputStream::ResultSet { this->outputDir_, this->baseName_ },
        this->reportIndex(report_step, time_step),
        EclIO::OutputStream::Formatted { this->es_.get().cfg().io().getFMTOUT() },
        EclIO::OutputStream::Unified   { this->es_.get().cfg().io().getUNIFOUT() }
    };

    RestartIO::save(rstFile, report_step, secs_elapsed,
                    std::move(value),
                    this->es_, this->grid_, this->schedule_,
                    action_state, wtest_state, st,
                    udq_state, this->aquiferData_, write_double);
}

void Opm::EclipseIO::Impl::writeRunSummary() const
{
    const auto outputFile = std::filesystem::path {
        this->outputDir_
    } / this->baseName_;

    EclIO::ESmry { outputFile.generic_string() }.write_rsm_file();
}

void Opm::EclipseIO::Impl::writeRftFile(const double       secs_elapsed,
                                        const int          report_step,
                                        const bool         haveExistingRFT,
                                        const data::Wells& wellSol) const
{
    // Open existing RFT file if report step is after first RFT event.
    const auto openExisting = EclIO::OutputStream::RFT::OpenExisting {
        haveExistingRFT
    };

    EclIO::OutputStream::RFT rftFile {
        EclIO::OutputStream::ResultSet { this->outputDir_, this->baseName_ },
        EclIO::OutputStream::Formatted { this->es_.get().cfg().io().getFMTOUT() },
        openExisting
    };

    RftIO::write(report_step,
                 secs_elapsed,
                 this->es_.get().getUnits(),
                 this->grid_,
                 this->schedule_,
                 wellSol,
                 rftFile);
}

// ---------------------------------------------------------------------------

void Opm::EclipseIO::Impl::writeInitFile(data::Solution                          simProps,
                                         std::map<std::string, std::vector<int>> int_data,
                                         const std::vector<NNCdata>&             nnc) const
{
    EclIO::OutputStream::Init initFile {
        EclIO::OutputStream::ResultSet { this->outputDir_, this->baseName_ },
        EclIO::OutputStream::Formatted { this->es_.get().cfg().io().getFMTOUT() }
    };

    simProps.convertFromSI(this->es_.get().getUnits());

    InitIO::write(this->es_, this->grid_, this->schedule_,
                  simProps, std::move(int_data), nnc, initFile);
}

void Opm::EclipseIO::Impl::writeEGridFile(const std::vector<NNCdata>& nnc) const
{
    const auto formatted = this->es_.get().cfg().io().getFMTOUT();

    const auto ext = '.'
        + (formatted ? std::string{"F"} : std::string{})
        + "EGRID";

    const auto egridFile = (std::filesystem::path{ this->outputDir_ }
        / (this->baseName_ + ext)).generic_string();

    this->grid_.save(egridFile, formatted, nnc, this->es_.get().getDeckUnitSystem());
}

void Opm::EclipseIO::Impl::recordSummaryOutput(const double secs_elapsed)
{
    if (this->sumthin_triggered_) {
        this->last_sumthin_output_ = secs_elapsed;
    }
}

int Opm::EclipseIO::Impl::reportIndex(const int                report_step,
                                      const std::optional<int> time_step) const
{
    return time_step.has_value()
        ?  (*time_step + 1)
        :  report_step;
}

bool Opm::EclipseIO::Impl::checkAndRecordIfSumthinTriggered(const int    report_step,
                                                            const double secs_elapsed) const
{
    const auto& sumthin = this->schedule_.get()[report_step - 1].sumthin();

    this->sumthin_active_ = sumthin.has_value(); // True if SUMTHIN value strictly positive.

    return this->sumthin_triggered_ = this->sumthin_active_
        && ! (secs_elapsed < this->last_sumthin_output_ + sumthin.value());
}

bool Opm::EclipseIO::Impl::summaryAtRptOnly(const int report_step) const
{
    return this->schedule_.get()[report_step - 1].rptonly();
}

// ===========================================================================

Opm::EclipseIO::EclipseIO(const EclipseState&  es,
                          EclipseGrid          grid,
                          const Schedule&      schedule,
                          const SummaryConfig& summary_config,
                          const std::string&   baseName,
                          const bool           writeEsmry)
    : impl { std::make_unique<Impl>(es, std::move(grid),
                                    schedule, summary_config,
                                    baseName, writeEsmry) }
{
    if (! this->impl->outputEnabled()) {
        return;
    }

    ensure_directory_exists(this->impl->outputDir());
}

Opm::EclipseIO::~EclipseIO() = default;

void Opm::EclipseIO::writeInitial(data::Solution                          simProps,
                                  std::map<std::string, std::vector<int>> int_data,
                                  const std::vector<NNCdata>&             nnc)
{
    if (! this->impl->outputEnabled()) {
        return;
    }

    this->impl->writeInitial(std::move(simProps), std::move(int_data), nnc);
}

void Opm::EclipseIO::writeTimeStep(const Action::State& action_state,
                                   const WellTestState& wtest_state,
                                   const SummaryState&  st,
                                   const UDQState&      udq_state,
                                   const int            report_step,
                                   const bool           isSubstep,
                                   const double         secs_elapsed,
                                   RestartValue         value,
                                   const bool           write_double,
                                   std::optional<int>   time_step)
{
    if (! this->impl->outputEnabled()) {
        // Run does not request any output.  Uncommon, but might be useful
        // in the case of performance testing.
        return;
    }

    // RFT file written only if requested and never for substeps.
    if (const auto& [wantRFT, haveExistingRFT] =
        this->impl->wantRFTOutput(report_step, isSubstep);
        wantRFT)
    {
        this->impl->writeRftFile(secs_elapsed, report_step,
                                 haveExistingRFT, value.wells);
    }

    if (this->impl->wantSummaryOutput(report_step, isSubstep, secs_elapsed, time_step)) {
        this->impl->writeSummaryFile(st, report_step, time_step,
                                     secs_elapsed, isSubstep);
    }

    if (this->impl->wantRestartOutput(report_step, isSubstep, time_step)) {
        // Restart file output (RPTRST &c).
        this->impl->writeRestartFile(action_state, wtest_state, st, udq_state,
                                     report_step, time_step, secs_elapsed,
                                     write_double, std::move(value));
    }

    if (! isSubstep &&
        this->impl->isFinalStep(report_step) &&
        this->impl->summaryConfig().createRunSummary())
    {
        // Write RSM file at end of simulation.
        this->impl->writeRunSummary();
    }
}

Opm::RestartValue
Opm::EclipseIO::loadRestart(Action::State&                 action_state,
                            SummaryState&                  summary_state,
                            const std::vector<RestartKey>& solution_keys,
                            const std::vector<RestartKey>& extra_keys) const
{
    return this->impl->loadRestart(solution_keys, extra_keys,
                                   action_state, summary_state);
}

Opm::data::Solution
Opm::EclipseIO::loadRestartSolution(const std::vector<RestartKey>& solution_keys,
                                    const int                      report_step) const
{
    return this->impl->loadRestartSolution(solution_keys, report_step);
}

const Opm::out::Summary& Opm::EclipseIO::summary() const
{
    return this->impl->summary();
}

const Opm::SummaryConfig& Opm::EclipseIO::finalSummaryConfig() const
{
    return this->impl->summaryConfig();
}
