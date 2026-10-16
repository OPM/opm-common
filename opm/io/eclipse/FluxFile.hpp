/*
   Copyright 2026 Equinor ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OPM_IO_FLUXFILE_HPP
#define OPM_IO_FLUXFILE_HPP

#include <limits>
#include <string>
#include <vector>

namespace Opm::EclIO {

class FluxFile
{
public:
    enum class Phase : int {
        Oil = 1,
        Water = 2,
        Gas = 4,
    };

    enum class Mode : int {
        Flux = 1,
        Pressure = 2,
        Both = 3,
    };

    enum class Sampling : int {
        Instant = 0,
        Averaged = 1,
    };

    struct Header {
        int version = formatVersion();
        int parentNx = 0;
        int parentNy = 0;
        int parentNz = 0;
        int boxI1 = 0;
        int boxJ1 = 0;
        int boxK1 = 0;
        int boxNx = 0;
        int boxNy = 0;
        int boxNz = 0;
        int numCells = 0;
        int numBoundaryFaces = 0;
        /// Number of boundary records in the file.
        ///
        /// Written as zero and derived from the blocks on read: the header
        /// goes down before the first record arrives and is never revisited.
        int numReportSteps = 0;
        int numPhases = 0;
        int phaseMask = 0;

        /// Whether any record carries exterior temperature.
        ///
        /// Derived on read, like numReportSteps.
        bool hasTemperature = false;
        Mode mode = Mode::Flux;
        Sampling sampling = Sampling::Averaged;

        /// Number of embedded parent summary vectors.  Matches
        /// Data::summaryKeys.size().
        int numSummaryKeys = 0;

        /// Number of embedded parent summary samples.  Matches
        /// Data::summarySamples.size().
        ///
        /// Derived on read, like numReportSteps.
        int numSummarySamples = 0;

        /// Whether the summary samples were taken at sub-report-step
        /// resolution.  False means summary data, if any, is only available
        /// at report-step boundaries.
        ///
        /// Derived on read, like numReportSteps.
        bool summaryPerTimestep = false;

        /// Minimum time, in seconds, that the producer enforced between
        /// consecutive summary samples.  Zero means every time step was
        /// sampled.  Diagnostic only.
        double summaryMinSampleInterval = 0.0;

        /// Whether the boundary records were written at sub-report-step
        /// resolution.  False means one record per report step.
        ///
        /// Derived on read, like numReportSteps.
        bool boundaryPerTimestep = false;

        /// Minimum time, in seconds, that the producer enforced between
        /// consecutive boundary records.  Zero means every time step was
        /// written.  Diagnostic only.
        double boundaryMinSampleInterval = 0.0;

        bool operator==(const Header& other) const;

        bool hasPhase(Phase phase) const
        {
            return (this->phaseMask & static_cast<int>(phase)) != 0;
        }
    };

    struct BoundaryFace {
        int interiorLocalCell = 0;
        int direction = 0;
        int exteriorGlobalCell = -1;
        double transmissibility = 0.0;

        /// PVT region of the cell on the far side of the face.
        ///
        /// A reduced run has no cell there, so without this it would have to
        /// substitute its own region when evaluating the density, formation
        /// volume factor and viscosity of an inflowing stream.
        ///
        /// Declared last so that existing aggregate initialisation of the
        /// preceding members keeps working.
        int exteriorPvtRegion = 0;

        /// Depth of the centre of the cell on the far side of the face.
        ///
        /// The pressures in a report step are that cell's, taken at its
        /// centre, while a boundary condition is imposed at the face. The two
        /// are not the same place, and in a dipping layer they are not even
        /// close: the consumer needs this depth to carry the pressure from the
        /// one to the other, or it imposes the exterior cell's pressure at a
        /// depth where the exterior cell does not have it.
        ///
        /// NaN when the producer did not record it, which is how a file
        /// written before this was stored reads back. A consumer that finds
        /// NaN cannot make the correction and should leave the pressure alone.
        double exteriorDepth = std::numeric_limits<double>::quiet_NaN();

        /// Equilibration region of the cell on the far side of the face.
        ///
        /// Paired with the interior cell's own region it selects an entry in
        /// Data::thresholdPressure.  Negative when the producer did not record
        /// it.
        int exteriorEquilRegion = -1;

        bool operator==(const BoundaryFace& other) const;
    };

    /// One record of sector boundary data.
    ///
    /// Records are not tied to the report step sequence: the producer writes
    /// one per time step, subject to a minimum interval between consecutive
    /// records, so several records may share the same \c reportStep.
    ///
    /// A record covers the half-open interval
    /// <tt>[startTime, startTime + stepLength]</tt>, in seconds.  The \c rates
    /// are the TIME-AVERAGE over that interval, so that holding them constant
    /// across it reproduces the flow over the interval exactly.  The state
    /// arrays -- \c pressures, \c swat, \c sgas, \c rs, \c rv and
    /// \c temperature -- are the exterior cell values at the END of the
    /// interval.
    ///
    /// Consumers should select a record by TIME rather than by report step
    /// index, which works for either cadence.
    struct ReportStep {
        int reportStep = 0;
        int simStep = 0;
        double startTime = 0.0;
        double stepLength = 0.0;
        std::vector<double> pressures;
        std::vector<double> swat;
        std::vector<double> sgas;
        std::vector<double> rs;
        std::vector<double> rv;
        std::vector<double> temperature;

        /// Component mass rates across each boundary face, flattened
        /// face-major then over the active phases in canonical Oil/Water/Gas
        /// order filtered by Header::phaseMask, positive into the sector.
        ///
        /// This is the whole of the Flux-mode payload. These are COMPONENT
        /// masses, not phase masses: the oil entry is the mass of the oil
        /// component in both phases and the gas entry is the mass of the gas
        /// component in both phases. The producing run forms them from its own
        /// flux and the UPWIND cell's inverse formation volume factor, Rs and
        /// Rv, using the INTERIOR cell's reference densities so that the
        /// consumer's conversion back to surface volumes is exact.
        ///
        /// The phase volumetric fluxes they were built from are deliberately
        /// not also stored. A consumer cannot use them: for flow entering the
        /// sector the upstream cell lies outside it, so there is no state with
        /// which to convert them, and splitting a phase flux by Rs/Rv after
        /// the fact needs exactly that state.
        std::vector<double> massRates;

        /// Relative permeability of the exterior cell, face-major over the
        /// active phases like \c rates.
        ///
        /// Written in Pressure mode. A reduced run needs these to form the
        /// mobility of an inflowing stream: evaluating the saturation
        /// functions of its own cell at the exterior saturations would use the
        /// wrong SATNUM region, the wrong scaled end points and none of the
        /// parent's hysteresis history.
        std::vector<double> relPerm;

        /// Capillary pressure of the exterior cell relative to the reference
        /// phase, same layout as \c relPerm, and written alongside it.
        std::vector<double> capPressure;

        /// Pore-volume weighted sums over the cells OUTSIDE the sector, used to
        /// rebuild the producing run's field averages.
        ///
        /// Sixteen values: eight weighted by hydrocarbon pore volume followed
        /// by eight weighted by total pore volume, each being pressure,
        /// temperature, rs, rv, rsw, rvw, pore volume and salt concentration,
        /// already multiplied by the weight and not yet divided by it.
        ///
        /// A reduced run adds these to its own sums before forming the
        /// averages that convert a reservoir volume target to surface rates.
        /// Without them it would average over its own cells only, and a well
        /// on RESV control would be given a different target than in the full
        /// model. Keeping sums rather than averages means the reduced run still
        /// reflects its own changes, such as a well it has added.
        std::vector<double> externalRegionSums;

        bool operator==(const ReportStep& other) const;
    };

    /// One snapshot of the parent run's summary vectors.
    ///
    /// Samples are independent of the report-step sequence: the producer
    /// emits one per time step, subject to a minimum interval between
    /// consecutive samples.
    struct SummarySample {
        /// End time, in seconds, of the interval this sample represents.
        double time = 0.0;

        /// One entry per Data::summaryKeys, in the same order.
        ///
        /// Entries whose keyword is of summary type Rate hold the
        /// TIME-AVERAGE over the interval since the previous sample (or
        /// since t = 0 for the first sample), so that holding the value
        /// piecewise-constant across that interval reproduces the parent's
        /// production over it exactly.  All other entries -- Total,
        /// Pressure, Ratio, ProdIndex, Mode and Count -- hold the
        /// instantaneous value at 'time'.
        std::vector<double> values;

        bool operator==(const SummarySample& other) const;
    };

    /// The producer's wells and groups over its report steps.
    ///
    /// Per-step arrays are step-major: entry <tt>s*n + i</tt> is well (or
    /// group) \c i at report step \c s. Steps are identified by their start
    /// time, so a consumer whose schedule has other report steps can still
    /// find the one that applies.
    struct ParentWells {
        /// Start of each of the producer's report steps, as POSIX time.
        std::vector<double> stepStart;

        std::vector<std::string> wells;

        /// Every group, FIELD included.
        std::vector<std::string> groups;

        /// One-based index into \c groups of the well's group; 0 before the
        /// well is defined.
        std::vector<int> wellGroup;

        /// 1 producer, 2 water injector, 3 gas injector, 4 oil injector.
        std::vector<int> wellType;

        /// 1 OPEN, 2 STOP, 3 SHUT, 4 AUTO.
        std::vector<int> wellStatus;

        std::vector<double> wellEfficiency;

        /// Reference depth of each well, in metres, NaN if it has none.
        std::vector<double> wellRefDepth;

        /// One-based index into \c groups of the group's parent; 0 for FIELD
        /// and before the group is defined.
        std::vector<int> groupParent;

        std::vector<double> groupEfficiency;

        bool empty() const { return this->wells.empty(); }

        bool operator==(const ParentWells& other) const = default;
    };

    struct Data {
        Header header;
        std::vector<std::string> names;
        std::vector<int> localToGlobal;
        std::vector<BoundaryFace> boundaryFaces;
        std::vector<std::string> summaryKeys;
        std::vector<ReportStep> reportSteps;
        std::vector<SummarySample> summarySamples;

        /// The producer's threshold pressures, as a flattened square matrix
        /// over its equilibration regions: entry <tt>r1*n + r2</tt> is the
        /// threshold for flow from region \c r1 to region \c r2, and \c n is
        /// the square root of the length.
        ///
        /// A THPRES entry given as defaulted is not a number in the deck: it
        /// is the largest initial potential difference found anywhere along
        /// that region boundary, so it can only be arrived at by equilibrating
        /// the whole field.  A sector holds part of each region boundary and
        /// would arrive at a smaller number, which is why it has to be handed
        /// the producer's rather than working out its own.
        ///
        /// Empty when the producer had no threshold pressures, or did not
        /// record them.
        std::vector<double> thresholdPressure;

        /// The producer's wells and groups, report step by report step.
        ///
        /// A sector cut out of the full model usually has only its own wells
        /// in its SCHEDULE section, and its group tree may lack groups that
        /// hold nothing but wells outside it. The embedded summary has those
        /// wells' rates, but not which group they belong to, whether they
        /// produce or inject, or how their rates are weighted, and without
        /// that a reduced run cannot count them into its group and field
        /// totals, its group controls or its UDQs.
        ///
        /// Empty when the producer did not record it.
        ParentWells parentWells;

        bool operator==(const Data& other) const;
    };

    static constexpr int formatVersion()
    {
        // 7: The boundary geometry arrays are named for what they describe:
        //    LOC2GLOB, FACECELL, FACEDIR and FACEGLNB, replacing LOCGLOB,
        //    FLUXCELL, FLUXDIR and FLUXNNC. FACEGLNB in particular holds the
        //    global neighbour of every face, not just of the NNC ones.
        // 6: FLXRATE dropped. The phase volumetric fluxes duplicated FLXMASS,
        //    which is what a consumer actually imposes, and could not be used
        //    on their own for inflow anyway.
        // 5: Record data is written as repeated self-contained blocks rather
        //    than one flat array per quantity spanning the whole run, so that
        //    records can be appended instead of the file being rewritten.
        // 4: FLXRCON added, the pore-volume weighted sums outside the sector.
        // 3: FLXMASS holds component masses. Version 2 wrote phase masses
        //    there, which a consumer cannot split by Rs/Rv, so those files
        //    are rejected rather than silently misread.
        return 7;
    }

    /// Incremental writer for a FLUX file.
    ///
    /// The static section -- header, names and boundary geometry -- goes down
    /// once, when the first data arrives. Everything after it is a
    /// self-contained block, so a record is added by appending to the end of
    /// the file rather than rewriting it. A run that writes N times therefore
    /// moves bytes proportional to N rather than to N squared, and the
    /// producer need not keep the whole history in memory.
    ///
    /// There is deliberately no whole-file write. Appending is the only way to
    /// produce a FLUX file, so no call can truncate the file of a run that is
    /// still going.
    class Writer
    {
    public:
        /// \param[in] filename Path to write.
        ///
        /// \param[in] formatted Whether to write text rather than binary.
        ///
        /// \param[in] staticData Header, names, cell map, boundary faces and
        ///    summary keys. Its \c reportSteps and \c summarySamples are
        ///    ignored; pass those to appendRecords() and
        ///    appendSummarySamples(). Nothing reaches the file until the first
        ///    of those calls, so the caller may still adjust the minimum
        ///    sample intervals and the summary keys until then.
        Writer(std::string filename, bool formatted, Data staticData);

        /// Append one block holding every record given.
        ///
        /// A quantity must be present on every record of a block or on none;
        /// records that lack one which others in the same block carry are
        /// padded with zeros.
        void appendRecords(const std::vector<ReportStep>& records);

        /// Append one block holding every summary sample given.
        void appendSummarySamples(const std::vector<SummarySample>& samples);

        /// Write the static section if nothing has been appended, so that a
        /// run which produced no records still leaves a readable file.
        void close();

        int numRecords() const { return this->numRecords_; }

    private:
        void writeStaticSection();

        std::string filename_;
        bool formatted_;
        Data static_;
        bool staticWritten_{false};
        int boundaryBlocks_{0};
        int summaryBlocks_{0};
        int numRecords_{0};
        int numSamples_{0};
        double lastSummaryTime_{-std::numeric_limits<double>::max()};
    };

    /// Read a FLUX file in full.
    ///
    /// A file whose last block was truncated, because the producing run was
    /// killed part way through a write, is read up to the last complete block
    /// rather than rejected.
    static Data read(const std::string& filename, bool preload = false);

    /// Read only the producer's well and group table, without the records.
    static ParentWells readParentWells(const std::string& filename);

private:
    static void validateStaticForWrite(const Data& data);
    static void validateRecordsForWrite(const Data& data,
                                        const std::vector<ReportStep>& records);
    static void validateSamplesForWrite(const Data& data,
                                        const std::vector<SummarySample>& samples,
                                        double previousTime);
    static void validateAfterRead(const Data& data);
};

} // namespace Opm::EclIO

#endif // OPM_IO_FLUXFILE_HPP