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

#include <opm/io/eclipse/FluxFile.hpp>

#include <opm/common/ErrorMacros.hpp>

#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/EclOutput.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace {

template <class Member>
using BoundaryMemberType = std::remove_cvref_t<decltype(std::declval<Opm::EclIO::FluxFile::BoundaryFace>().*std::declval<Member>())>;

template <class Member>
using StepMemberType = std::remove_cvref_t<decltype(std::declval<Opm::EclIO::FluxFile::ReportStep>().*std::declval<Member>())>;

constexpr int hasTemperatureIndex = 14;
constexpr int modeIndex = 15;
constexpr int samplingIndex = 16;
constexpr int phaseMaskIndex = 17;
constexpr int numSummaryKeysIndex = 18;
constexpr int numSummarySamplesIndex = 19;
constexpr int summaryPerTimestepIndex = 20;
constexpr int boundaryPerTimestepIndex = 21;
constexpr int headerSize = 22;

std::vector<int> makeHeader(const Opm::EclIO::FluxFile::Header& header)
{
    return {
        header.version,
        header.parentNx,
        header.parentNy,
        header.parentNz,
        header.boxI1,
        header.boxJ1,
        header.boxK1,
        header.boxNx,
        header.boxNy,
        header.boxNz,
        header.numCells,
        header.numBoundaryFaces,
        header.numReportSteps,
        header.numPhases,
        header.hasTemperature ? 1 : 0,
        static_cast<int>(header.mode),
        static_cast<int>(header.sampling),
        header.phaseMask,
        header.numSummaryKeys,
        header.numSummarySamples,
        header.summaryPerTimestep ? 1 : 0,
        header.boundaryPerTimestep ? 1 : 0,
    };
}

Opm::EclIO::FluxFile::Header parseHeader(const std::vector<int>& values)
{
    if (values.size() < headerSize) {
        OPM_THROW(std::runtime_error,
                  fmt::format("FLUXHEAD must contain at least {} integers, got {}",
                              headerSize, values.size()));
    }

    Opm::EclIO::FluxFile::Header header;
    header.version = values[0];
    header.parentNx = values[1];
    header.parentNy = values[2];
    header.parentNz = values[3];
    header.boxI1 = values[4];
    header.boxJ1 = values[5];
    header.boxK1 = values[6];
    header.boxNx = values[7];
    header.boxNy = values[8];
    header.boxNz = values[9];
    header.numCells = values[10];
    header.numBoundaryFaces = values[11];
    header.numReportSteps = values[12];
    header.numPhases = values[13];
    header.hasTemperature = values[hasTemperatureIndex] != 0;
    header.mode = static_cast<Opm::EclIO::FluxFile::Mode>(values[modeIndex]);
    header.sampling = static_cast<Opm::EclIO::FluxFile::Sampling>(values[samplingIndex]);
    header.phaseMask = values[phaseMaskIndex];
    header.numSummaryKeys = values[numSummaryKeysIndex];
    header.numSummarySamples = values[numSummarySamplesIndex];
    header.summaryPerTimestep = values[summaryPerTimestepIndex] != 0;
    header.boundaryPerTimestep = values[boundaryPerTimestepIndex] != 0;
    return header;
}

template <class T>
void requireArray(Opm::EclIO::EclFile& file, const std::string& name)
{
    if (!file.hasKey(name)) {
        OPM_THROW(std::runtime_error, fmt::format("Missing required FLUX array '{}'", name));
    }

    file.template get<T>(name);
}

template <class T>
const std::vector<T>& optionalArray(Opm::EclIO::EclFile& file, const std::string& name)
{
    static const std::vector<T> empty;
    if (!file.hasKey(name)) {
        return empty;
    }

    return file.template get<T>(name);
}

template <class Member>
std::vector<BoundaryMemberType<Member>> flattenBoundary(
    const std::vector<Opm::EclIO::FluxFile::BoundaryFace>& faces,
    Member member)
{
    std::vector<BoundaryMemberType<Member>> values;
    values.reserve(faces.size());
    for (const auto& face : faces) {
        values.push_back(face.*member);
    }
    return values;
}

template <class Member>
std::vector<StepMemberType<Member>> flattenStepMeta(
    const std::vector<Opm::EclIO::FluxFile::ReportStep>& steps,
    Member member)
{
    std::vector<StepMemberType<Member>> values;
    values.reserve(steps.size());
    for (const auto& step : steps) {
        values.push_back(step.*member);
    }
    return values;
}

// A block stores one array per quantity covering all of its records, so a
// quantity has to be present on every record of the block or on none. Records
// that lack one the others carry are padded, rather than the whole series being
// dropped: the first record is emitted before anything has been sampled.
std::vector<double> flattenVectorsPadded(const std::vector<Opm::EclIO::FluxFile::ReportStep>& steps,
                                         const std::vector<double> Opm::EclIO::FluxFile::ReportStep::* member,
                                         const std::size_t width)
{
    std::vector<double> values;
    values.reserve(steps.size() * width);
    for (const auto& step : steps) {
        const auto& field = step.*member;
        if (field.empty()) {
            values.insert(values.end(), width, 0.0);
        }
        else {
            values.insert(values.end(), field.begin(), field.end());
        }
    }
    return values;
}

std::vector<std::string> makeNames(const Opm::EclIO::FluxFile::Data& data)
{
    auto names = data.names;
    names.insert(names.end(), data.summaryKeys.begin(), data.summaryKeys.end());
    return names;
}

std::vector<double>
flattenSummaryTimes(const std::vector<Opm::EclIO::FluxFile::SummarySample>& samples)
{
    std::vector<double> times;
    times.reserve(samples.size());
    for (const auto& sample : samples) {
        times.push_back(sample.time);
    }
    return times;
}

std::vector<double>
flattenSummaryValues(const std::vector<Opm::EclIO::FluxFile::SummarySample>& samples)
{
    std::vector<double> values;
    for (const auto& sample : samples) {
        values.insert(values.end(), sample.values.begin(), sample.values.end());
    }
    return values;
}

bool hasAnyValues(const std::vector<Opm::EclIO::FluxFile::ReportStep>& steps,
                  const std::vector<double> Opm::EclIO::FluxFile::ReportStep::* member)
{
    for (const auto& step : steps) {
        if (!(step.*member).empty()) {
            return true;
        }
    }

    return false;
}

Opm::EclIO::EclFile openFluxFile(const std::string& filename, bool preload)
{
    // Tolerant: a FLUX file grows by appending as the producing run proceeds,
    // so one that was killed part way through a write ends inside an array.
    // Read up to the last complete one rather than refusing the file.
    using Formatted = Opm::EclIO::EclFile::Formatted;
    using Tolerant = Opm::EclIO::EclFile::Tolerant;

    // Tolerant mode stops rather than throwing on something it cannot parse,
    // so whether the file is binary is settled by looking for the array every
    // FLUX file opens with, not by catching an exception.
    const auto isBinary = [&filename]
    {
        const Opm::EclIO::EclFile probe{filename, Formatted{false}, Tolerant{true}, false};
        const auto& names = probe.arrayNames();
        return !names.empty() && (names.front() == "FLUXHEAD");
    }();

    return {filename, Formatted{!isBinary}, Tolerant{true}, preload};
}

void checkParentWells(const Opm::EclIO::FluxFile::ParentWells& wells, const std::string& context)
{
    if (wells.empty()) {
        return;
    }

    const auto numSteps = wells.stepStart.size();
    const auto perWell = numSteps * wells.wells.size();
    const auto perGroup = numSteps * wells.groups.size();

    const auto check = [&context](const std::size_t size, const std::size_t expected,
                                  const char* name)
    {
        if (size != expected) {
            OPM_THROW(std::runtime_error,
                      fmt::format("{}: {} holds {} values, expected {}",
                                  context, name, size, expected));
        }
    };

    check(wells.wellGroup.size(), perWell, "FLXWGRP");
    check(wells.wellType.size(), perWell, "FLXWTYPE");
    check(wells.wellStatus.size(), perWell, "FLXWSTAT");
    check(wells.wellEfficiency.size(), perWell, "FLXWEFAC");
    check(wells.wellRefDepth.size(), wells.wells.size(), "FLXWDREF");
    check(wells.groupParent.size(), perGroup, "FLXGPAR");
    check(wells.groupEfficiency.size(), perGroup, "FLXGEFAC");

    const auto numGroups = static_cast<int>(wells.groups.size());
    const auto inRange = [numGroups](const int index) { return (index >= 0) && (index <= numGroups); };
    if (!std::all_of(wells.wellGroup.begin(), wells.wellGroup.end(), inRange)
        || !std::all_of(wells.groupParent.begin(), wells.groupParent.end(), inRange))
    {
        OPM_THROW(std::runtime_error,
                  fmt::format("{}: a well or group refers to a group that is not listed", context));
    }
}

Opm::EclIO::FluxFile::ParentWells
readParentWellsFrom(Opm::EclIO::EclFile& file, const std::string& filename)
{
    Opm::EclIO::FluxFile::ParentWells wells;
    if (!file.hasKey("FLXWELLS")) {
        return wells;
    }

    wells.stepStart = optionalArray<double>(file, "FLXPSTRT");
    wells.wells = optionalArray<std::string>(file, "FLXWELLS");
    wells.groups = optionalArray<std::string>(file, "FLXGRUPS");
    wells.wellGroup = optionalArray<int>(file, "FLXWGRP");
    wells.wellType = optionalArray<int>(file, "FLXWTYPE");
    wells.wellStatus = optionalArray<int>(file, "FLXWSTAT");
    wells.wellEfficiency = optionalArray<double>(file, "FLXWEFAC");
    wells.wellRefDepth = optionalArray<double>(file, "FLXWDREF");
    wells.groupParent = optionalArray<int>(file, "FLXGPAR");
    wells.groupEfficiency = optionalArray<double>(file, "FLXGEFAC");

    checkParentWells(wells, fmt::format("FLUX file '{}'", filename));
    return wells;
}

} // namespace

namespace Opm::EclIO {

bool FluxFile::Header::operator==(const Header& other) const = default;

bool FluxFile::BoundaryFace::operator==(const BoundaryFace& other) const
{
    // An unrecorded exterior depth is held as NaN, which is not equal to
    // itself, so two faces that both lack one would otherwise never compare
    // equal -- and a file that records no depths would not survive a round
    // trip.
    const auto sameDepth = (std::isnan(this->exteriorDepth) &&
                            std::isnan(other.exteriorDepth))
        || (this->exteriorDepth == other.exteriorDepth);

    return (this->interiorLocalCell == other.interiorLocalCell)
        && (this->direction == other.direction)
        && (this->exteriorGlobalCell == other.exteriorGlobalCell)
        && (this->transmissibility == other.transmissibility)
        && (this->exteriorPvtRegion == other.exteriorPvtRegion)
        && (this->exteriorEquilRegion == other.exteriorEquilRegion)
        && sameDepth;
}

bool FluxFile::ParentWells::operator==(const ParentWells& other) const
{
    // A well without a reference depth is held as NaN, as for BoundaryFace.
    const auto sameDepths = std::equal(this->wellRefDepth.begin(), this->wellRefDepth.end(),
                                       other.wellRefDepth.begin(), other.wellRefDepth.end(),
                                       [](const double a, const double b)
                                       { return (std::isnan(a) && std::isnan(b)) || (a == b); });

    return (this->stepStart == other.stepStart)
        && (this->wells == other.wells)
        && (this->groups == other.groups)
        && (this->wellGroup == other.wellGroup)
        && (this->wellType == other.wellType)
        && (this->wellStatus == other.wellStatus)
        && (this->wellEfficiency == other.wellEfficiency)
        && sameDepths
        && (this->groupParent == other.groupParent)
        && (this->groupEfficiency == other.groupEfficiency);
}

bool FluxFile::ReportStep::operator==(const ReportStep& other) const = default;

bool FluxFile::SummarySample::operator==(const SummarySample& other) const = default;

bool FluxFile::Data::operator==(const Data& other) const = default;

FluxFile::Writer::Writer(std::string filename, bool formatted, Data staticData)
    : filename_(std::move(filename))
    , formatted_(formatted)
    , static_(std::move(staticData))
{
    // The records the caller happened to leave in there are not ours to write.
    this->static_.reportSteps.clear();
    this->static_.summarySamples.clear();
}

void FluxFile::Writer::writeStaticSection()
{
    validateStaticForWrite(this->static_);

    const auto& data = this->static_;

    // Counts and cadence flags describe data that has not been written yet, so
    // they go down as zero and the reader derives them from the blocks it
    // finds. Patching them afterwards would mean seeking back into a file the
    // producer is still appending to.
    auto header = data.header;
    header.numReportSteps = 0;
    header.numSummarySamples = 0;
    header.hasTemperature = false;
    header.boundaryPerTimestep = false;
    header.summaryPerTimestep = false;

    EclOutput output(this->filename_, this->formatted_);
    output.write("FLUXHEAD", makeHeader(header));
    output.write("FLUXNAMS", makeNames(data), 32);
    output.write("FLUXNCNT", std::vector<int>{static_cast<int>(data.names.size()),
                                              static_cast<int>(data.summaryKeys.size())});
    output.write("LOC2GLOB", data.localToGlobal);
    output.write("FACECELL", flattenBoundary(data.boundaryFaces, &BoundaryFace::interiorLocalCell));
    output.write("FACEDIR", flattenBoundary(data.boundaryFaces, &BoundaryFace::direction));
    output.write("FACEGLNB", flattenBoundary(data.boundaryFaces, &BoundaryFace::exteriorGlobalCell));
    output.write("FLUXTRAN", flattenBoundary(data.boundaryFaces, &BoundaryFace::transmissibility));
    output.write("FLXPVTN", flattenBoundary(data.boundaryFaces, &BoundaryFace::exteriorPvtRegion));
    output.write("FLXEXDP", flattenBoundary(data.boundaryFaces, &BoundaryFace::exteriorDepth));
    output.write("FLXEQLN", flattenBoundary(data.boundaryFaces, &BoundaryFace::exteriorEquilRegion));

    if (!data.thresholdPressure.empty()) {
        output.write("FLXTHPR", data.thresholdPressure);
    }
    output.write("FLXMINT", std::vector<double>{data.header.boundaryMinSampleInterval});

    if (const auto& wells = data.parentWells; !wells.empty()) {
        output.write("FLXPSTRT", wells.stepStart);
        output.write("FLXWELLS", wells.wells, 32);
        output.write("FLXGRUPS", wells.groups, 32);
        output.write("FLXWGRP", wells.wellGroup);
        output.write("FLXWTYPE", wells.wellType);
        output.write("FLXWSTAT", wells.wellStatus);
        output.write("FLXWEFAC", wells.wellEfficiency);
        output.write("FLXWDREF", wells.wellRefDepth);
        output.write("FLXGPAR", wells.groupParent);
        output.write("FLXGEFAC", wells.groupEfficiency);
    }

    if (!data.summaryKeys.empty()) {
        output.write("SMRYMINT", std::vector<double>{data.header.summaryMinSampleInterval});
    }

    output.flushStream();

    this->staticWritten_ = true;
}

void FluxFile::Writer::appendRecords(const std::vector<ReportStep>& records)
{
    if (records.empty()) {
        return;
    }

    validateRecordsForWrite(this->static_, records);

    if (!this->staticWritten_) {
        this->writeStaticSection();
    }

    const auto& header = this->static_.header;
    const auto perFaceValues = static_cast<std::size_t>(header.numBoundaryFaces);
    const auto perFacePhaseValues = perFaceValues * static_cast<std::size_t>(header.numPhases);
    constexpr auto externalRegionSumCount = std::size_t{16};

    EclOutput output(this->filename_, this->formatted_, std::ios::app);

    // FLXSEQ opens the block and says how many records it holds, the way SEQNUM
    // opens a restart step.
    output.write("FLXSEQ", std::vector<int>{this->boundaryBlocks_,
                                            static_cast<int>(records.size())});
    output.write("FLXSTEP", flattenStepMeta(records, &ReportStep::reportStep));
    output.write("FLXSIM", flattenStepMeta(records, &ReportStep::simStep));
    output.write("FLXTIME", flattenStepMeta(records, &ReportStep::startTime));
    output.write("FLXDT", flattenStepMeta(records, &ReportStep::stepLength));

    const auto emit = [&output, &records]
        (const char* name,
         const std::vector<double> ReportStep::* member,
         const std::size_t width)
    {
        if (hasAnyValues(records, member)) {
            output.write(name, flattenVectorsPadded(records, member, width));
        }
    };

    if ((static_cast<int>(header.mode) & static_cast<int>(Mode::Flux)) != 0) {
        output.write("FLXMASS", flattenVectorsPadded(records, &ReportStep::massRates,
                                                     perFacePhaseValues));
    }

    if ((static_cast<int>(header.mode) & static_cast<int>(Mode::Pressure)) != 0) {
        output.write("FLXPRES", flattenVectorsPadded(records, &ReportStep::pressures,
                                                     perFaceValues));

        if (header.hasPhase(Phase::Water)) {
            output.write("FLXSATW", flattenVectorsPadded(records, &ReportStep::swat,
                                                         perFaceValues));
        }

        if (header.hasPhase(Phase::Gas)) {
            output.write("FLXSATG", flattenVectorsPadded(records, &ReportStep::sgas,
                                                         perFaceValues));
        }

        emit("FLXRS", &ReportStep::rs, perFaceValues);
        emit("FLXRV", &ReportStep::rv, perFaceValues);
        emit("FLXTEMP", &ReportStep::temperature, perFaceValues);
        emit("FLXKR", &ReportStep::relPerm, perFacePhaseValues);
        emit("FLXPC", &ReportStep::capPressure, perFacePhaseValues);
    }

    emit("FLXRCON", &ReportStep::externalRegionSums, externalRegionSumCount);

    output.flushStream();

    ++this->boundaryBlocks_;
    this->numRecords_ += static_cast<int>(records.size());
}

void FluxFile::Writer::appendSummarySamples(const std::vector<SummarySample>& samples)
{
    if (samples.empty()) {
        return;
    }

    validateSamplesForWrite(this->static_, samples, this->lastSummaryTime_);

    if (!this->staticWritten_) {
        this->writeStaticSection();
    }

    EclOutput output(this->filename_, this->formatted_, std::ios::app);

    output.write("SMRYSEQ", std::vector<int>{this->summaryBlocks_,
                                             static_cast<int>(samples.size())});
    output.write("SMRYTIME", flattenSummaryTimes(samples));
    output.write("SMRYVALS", flattenSummaryValues(samples));

    output.flushStream();

    ++this->summaryBlocks_;
    this->numSamples_ += static_cast<int>(samples.size());
    this->lastSummaryTime_ = samples.back().time;
}

void FluxFile::Writer::close()
{
    if (!this->staticWritten_) {
        this->writeStaticSection();
    }
}

FluxFile::Data FluxFile::read(const std::string& filename, bool preload)
{
    EclFile file = openFluxFile(filename, preload);

    requireArray<int>(file, "FLUXHEAD");
    requireArray<std::string>(file, "FLUXNAMS");
    requireArray<int>(file, "FLUXNCNT");
    requireArray<int>(file, "LOC2GLOB");
    requireArray<int>(file, "FACECELL");
    requireArray<int>(file, "FACEDIR");
    requireArray<int>(file, "FACEGLNB");
    requireArray<double>(file, "FLUXTRAN");

    Data data;
    data.header = parseHeader(file.get<int>("FLUXHEAD"));

    if (data.header.version != formatVersion()) {
        OPM_THROW(std::runtime_error,
                  fmt::format("Unsupported FLUXHEAD version {} in '{}'; expected {}",
                              data.header.version, filename, formatVersion()));
    }

    const auto& fluxnams = file.get<std::string>("FLUXNAMS");
    const auto& nameCounts = file.get<int>("FLUXNCNT");
    if (nameCounts.size() != 2) {
        OPM_THROW(std::runtime_error,
                  fmt::format("FLUXNCNT must contain 2 integers, got {}", nameCounts.size()));
    }

    const auto numNames = static_cast<std::size_t>(nameCounts[0]);
    const auto numSummaryKeys = static_cast<std::size_t>(nameCounts[1]);
    if (fluxnams.size() != numNames + numSummaryKeys) {
        OPM_THROW(std::runtime_error,
                  fmt::format("FLUXNAMS size {} does not match FLUXNCNT ({}, {})",
                              fluxnams.size(), numNames, numSummaryKeys));
    }

    data.names.assign(fluxnams.begin(), fluxnams.begin() + numNames);
    data.summaryKeys.assign(fluxnams.begin() + numNames, fluxnams.end());
    data.localToGlobal = file.get<int>("LOC2GLOB");

    const auto& fluxCell = file.get<int>("FACECELL");
    const auto& fluxDir = file.get<int>("FACEDIR");
    const auto& fluxNnc = file.get<int>("FACEGLNB");
    const auto& fluxTran = file.get<double>("FLUXTRAN");
    if (!(fluxCell.size() == fluxDir.size() && fluxDir.size() == fluxNnc.size() && fluxNnc.size() == fluxTran.size())) {
        OPM_THROW(std::runtime_error, "Boundary face arrays in FLUX file have inconsistent sizes");
    }

    data.boundaryFaces.reserve(fluxCell.size());
    const auto& fluxPvtn = optionalArray<int>(file, "FLXPVTN");
    const auto& fluxExDp = optionalArray<double>(file, "FLXEXDP");
    const auto& fluxEqln = optionalArray<int>(file, "FLXEQLN");
    for (std::size_t index = 0; index < fluxCell.size(); ++index) {
        data.boundaryFaces.push_back(BoundaryFace{fluxCell[index], fluxDir[index], fluxNnc[index],
                                                  fluxTran[index],
                                                  (index < fluxPvtn.size()) ? fluxPvtn[index] : 0,
                                                  (index < fluxExDp.size())
                                                  ? fluxExDp[index]
                                                  : std::numeric_limits<double>::quiet_NaN(),
                                                  (index < fluxEqln.size()) ? fluxEqln[index] : -1});
    }

    data.thresholdPressure = optionalArray<double>(file, "FLXTHPR");
    data.parentWells = readParentWellsFrom(file, filename);

    const auto& summaryMinInterval = optionalArray<double>(file, "SMRYMINT");
    const auto& boundaryMinInterval = optionalArray<double>(file, "FLXMINT");

    data.header.summaryMinSampleInterval =
        summaryMinInterval.empty() ? 0.0 : summaryMinInterval.front();

    data.header.boundaryMinSampleInterval =
        boundaryMinInterval.empty() ? 0.0 : boundaryMinInterval.front();

    // Everything from here on lives in self-contained blocks appended after the
    // static section: FLXSEQ opens a block of boundary records and SMRYSEQ one
    // of summary samples, each running to the next marker or to the end.
    const auto& arrayNames = file.arrayNames();

    const auto isMarker = [](const std::string& name)
    {
        return (name == "FLXSEQ") || (name == "SMRYSEQ");
    };

    const auto perFaceValues = static_cast<std::size_t>(data.header.numBoundaryFaces);
    const auto perFacePhaseValues = perFaceValues * static_cast<std::size_t>(data.header.numPhases);
    const auto perSummaryValues = data.summaryKeys.size();
    constexpr auto externalRegionSumCount = std::size_t{16};

    // A file the producer was killed part way through ends inside an array, and
    // the reader above stopped at the last complete one. The block that array
    // belonged to is then only partly there, so it is dropped whole: a torn
    // file loses its last block and nothing else.
    std::size_t recordsBeforeLastBlock = 0;
    std::size_t samplesBeforeLastBlock = 0;
    bool sawBlock = false;

    for (std::size_t first = 0; first < arrayNames.size(); ) {
        if (!isMarker(arrayNames[first])) {
            ++first;
            continue;
        }

        auto last = first + 1;
        while ((last < arrayNames.size()) && !isMarker(arrayNames[last])) {
            ++last;
        }

        const auto isFinalBlock = (last == arrayNames.size());

        recordsBeforeLastBlock = data.reportSteps.size();
        samplesBeforeLastBlock = data.summarySamples.size();
        sawBlock = true;

        // Index of a named array within this block, or -1.
        const auto indexOf = [&arrayNames, first, last](const std::string& name) -> int
        {
            for (auto k = first; k < last; ++k) {
                if (arrayNames[k] == name) {
                    return static_cast<int>(k);
                }
            }

            return -1;
        };

        try {
            const auto& marker = file.get<int>(static_cast<int>(first));
            if (marker.size() < 2) {
                OPM_THROW(std::runtime_error,
                          fmt::format("{} must contain 2 integers, got {}",
                                      arrayNames[first], marker.size()));
            }

            if (marker[1] < 0) {
                OPM_THROW(std::runtime_error,
                          fmt::format("{} declares a negative count {}",
                                      arrayNames[first], marker[1]));
            }

            const auto count = static_cast<std::size_t>(marker[1]);

            if (arrayNames[first] == "FLXSEQ") {
                const auto metaIndex = [&](const std::string& name)
                {
                    const auto idx = indexOf(name);
                    if (idx < 0) {
                        OPM_THROW(std::runtime_error,
                                  fmt::format("Missing required FLUX array '{}'", name));
                    }

                    return idx;
                };

                const auto requireCount = [count](const std::string& name, std::size_t got)
                {
                    if (got != count) {
                        OPM_THROW(std::runtime_error,
                                  fmt::format("{} holds {} values but the block declares {} "
                                              "records", name, got, count));
                    }
                };

                const auto& stepNumbers = file.get<int>(metaIndex("FLXSTEP"));
                const auto& simNumbers = file.get<int>(metaIndex("FLXSIM"));
                const auto& startTimes = file.get<double>(metaIndex("FLXTIME"));
                const auto& stepLengths = file.get<double>(metaIndex("FLXDT"));

                requireCount("FLXSTEP", stepNumbers.size());
                requireCount("FLXSIM", simNumbers.size());
                requireCount("FLXTIME", startTimes.size());
                requireCount("FLXDT", stepLengths.size());

                const auto base = data.reportSteps.size();
                data.reportSteps.resize(base + count);

                for (std::size_t r = 0; r < count; ++r) {
                    auto& step = data.reportSteps[base + r];
                    step.reportStep = stepNumbers[r];
                    step.simStep = simNumbers[r];
                    step.startTime = startTimes[r];
                    step.stepLength = stepLengths[r];
                }

                const auto split = [&](const std::string& name,
                                       std::vector<double> ReportStep::* member,
                                       const std::size_t width)
                {
                    const auto idx = indexOf(name);
                    if (idx < 0) {
                        return;
                    }

                    const auto& flat = file.get<double>(idx);
                    if (flat.size() != width * count) {
                        OPM_THROW(std::runtime_error,
                                  fmt::format("{} size {} does not match {} values per record "
                                              "x {} records", name, flat.size(), width, count));
                    }

                    for (std::size_t r = 0; r < count; ++r) {
                        const auto begin = flat.begin() + static_cast<std::ptrdiff_t>(r * width);
                        (data.reportSteps[base + r].*member)
                            .assign(begin, begin + static_cast<std::ptrdiff_t>(width));
                    }
                };

                split("FLXMASS", &ReportStep::massRates, perFacePhaseValues);
                split("FLXKR", &ReportStep::relPerm, perFacePhaseValues);
                split("FLXPC", &ReportStep::capPressure, perFacePhaseValues);
                split("FLXRCON", &ReportStep::externalRegionSums, externalRegionSumCount);
                split("FLXPRES", &ReportStep::pressures, perFaceValues);
                split("FLXSATW", &ReportStep::swat, perFaceValues);
                split("FLXSATG", &ReportStep::sgas, perFaceValues);
                split("FLXRS", &ReportStep::rs, perFaceValues);
                split("FLXRV", &ReportStep::rv, perFaceValues);
                split("FLXTEMP", &ReportStep::temperature, perFaceValues);
            }
            else {
                if (perSummaryValues == 0) {
                    OPM_THROW(std::runtime_error,
                              "FLUX file contains summary samples but no summary keys");
                }

                const auto timeIdx = indexOf("SMRYTIME");
                const auto valueIdx = indexOf("SMRYVALS");
                if ((timeIdx < 0) || (valueIdx < 0)) {
                    OPM_THROW(std::runtime_error,
                              "Summary block is missing SMRYTIME or SMRYVALS");
                }

                const auto& times = file.get<double>(timeIdx);
                const auto& values = file.get<double>(valueIdx);

                if (times.size() != count) {
                    OPM_THROW(std::runtime_error,
                              fmt::format("SMRYTIME holds {} values but the block declares {} "
                                          "samples", times.size(), count));
                }

                if (values.size() != perSummaryValues * count) {
                    OPM_THROW(std::runtime_error,
                              fmt::format("SMRYVALS size {} does not match {} keys x {} samples",
                                          values.size(), perSummaryValues, count));
                }

                const auto base = data.summarySamples.size();
                data.summarySamples.resize(base + count);
                for (std::size_t s = 0; s < count; ++s) {
                    const auto begin = values.begin()
                        + static_cast<std::ptrdiff_t>(s * perSummaryValues);

                    data.summarySamples[base + s].time = times[s];
                    data.summarySamples[base + s].values
                        .assign(begin, begin + static_cast<std::ptrdiff_t>(perSummaryValues));
                }
            }
        }
        catch (const std::exception& e) {
            // A block that is short or unreadable at the very end of the file is
            // what a producer killed part way through a write leaves behind.
            // Keep everything before it. Anywhere else it means real damage.
            if (!isFinalBlock) {
                throw;
            }

            OpmLog::warning(
                fmt::format("The last block of '{}' is incomplete and has been discarded: {}. "
                            "This is what a run killed during a write leaves behind; the "
                            "records before it are intact.", filename, e.what()));

            data.reportSteps.resize(recordsBeforeLastBlock);
            data.summarySamples.resize(samplesBeforeLastBlock);
            break;
        }

        first = last;
    }

    if (file.isTruncated() && sawBlock) {
        // The array chain itself ran out, so the block the reader just finished
        // was missing whatever came after the tear even if it assembled without
        // complaint. Drop it rather than hand back a record with holes in it.
        if ((data.reportSteps.size() > recordsBeforeLastBlock)
            || (data.summarySamples.size() > samplesBeforeLastBlock))
        {
            OpmLog::warning(
                fmt::format("'{}' ends part way through a block, which is what a run killed "
                            "during a write leaves behind. That block has been discarded; "
                            "the {} record(s) before it are intact.",
                            filename, recordsBeforeLastBlock));
        }

        data.reportSteps.resize(recordsBeforeLastBlock);
        data.summarySamples.resize(samplesBeforeLastBlock);
    }

    // The header goes down before any of this exists, so its counts and cadence
    // flags are recovered from the blocks rather than read from the file.
    data.header.numReportSteps = static_cast<int>(data.reportSteps.size());
    data.header.numSummarySamples = static_cast<int>(data.summarySamples.size());
    data.header.summaryPerTimestep = !data.summarySamples.empty();

    data.header.hasTemperature =
        std::any_of(data.reportSteps.begin(), data.reportSteps.end(),
                    [](const ReportStep& step) { return !step.temperature.empty(); });

    data.header.boundaryPerTimestep = false;
    for (std::size_t step = 1; step < data.reportSteps.size(); ++step) {
        if (data.reportSteps[step].reportStep == data.reportSteps[step - 1].reportStep) {
            data.header.boundaryPerTimestep = true;
            break;
        }
    }

    validateAfterRead(data);
    return data;
}

FluxFile::ParentWells FluxFile::readParentWells(const std::string& filename)
{
    EclFile file = openFluxFile(filename, /*preload=*/false);
    return readParentWellsFrom(file, filename);
}

void FluxFile::validateStaticForWrite(const Data& data)
{
    if (data.header.version != formatVersion()) {
        OPM_THROW(std::invalid_argument,
                  fmt::format("FluxFile writer only supports version {}, got {}",
                              formatVersion(), data.header.version));
    }

    if (data.header.numCells != static_cast<int>(data.localToGlobal.size())) {
        OPM_THROW(std::invalid_argument,
                  fmt::format("Header numCells {} does not match LOC2GLOB size {}",
                              data.header.numCells, data.localToGlobal.size()));
    }

    if (data.header.numBoundaryFaces != static_cast<int>(data.boundaryFaces.size())) {
        OPM_THROW(std::invalid_argument,
                  fmt::format("Header numBoundaryFaces {} does not match boundary face count {}",
                              data.header.numBoundaryFaces, data.boundaryFaces.size()));
    }

    if (data.header.numPhases != std::popcount(static_cast<unsigned int>(data.header.phaseMask))) {
        OPM_THROW(std::invalid_argument,
                  fmt::format("Header numPhases {} does not match phaseMask 0x{:x}",
                              data.header.numPhases, data.header.phaseMask));
    }

    if (data.header.numSummaryKeys != static_cast<int>(data.summaryKeys.size())) {
        OPM_THROW(std::invalid_argument,
                  fmt::format("Header numSummaryKeys {} does not match summary key count {}",
                              data.header.numSummaryKeys, data.summaryKeys.size()));
    }

    checkParentWells(data.parentWells, "FluxFile writer");
}

void FluxFile::validateRecordsForWrite(const Data& data,
                                       const std::vector<ReportStep>& records)
{
    const auto fluxEnabled = (static_cast<int>(data.header.mode) & static_cast<int>(Mode::Flux)) != 0;
    const auto pressureEnabled = (static_cast<int>(data.header.mode) & static_cast<int>(Mode::Pressure)) != 0;
    const auto perFaceValues = static_cast<std::size_t>(data.header.numBoundaryFaces);
    const auto perFacePhaseValues = perFaceValues * static_cast<std::size_t>(data.header.numPhases);

    for (const auto& step : records) {
        if (fluxEnabled && step.massRates.size() != perFacePhaseValues) {
            OPM_THROW(std::invalid_argument,
                      fmt::format("Each FLXMASS step must contain {} values, got {}",
                                  perFacePhaseValues, step.massRates.size()));
        }

        if (!step.externalRegionSums.empty() && (step.externalRegionSums.size() != 16)) {
            OPM_THROW(std::invalid_argument,
                      fmt::format("Each FLXRCON step must contain 16 values, got {}",
                                  step.externalRegionSums.size()));
        }

        if (pressureEnabled) {
            const auto checkSize = [](const std::vector<double>& values,
                                      const std::string& name,
                                      const std::size_t expected)
            {
                if (values.size() != expected) {
                    OPM_THROW(std::invalid_argument,
                              fmt::format("Each {} step must contain {} values, got {}",
                                          name, expected, values.size()));
                }
            };

            checkSize(step.pressures, "FLXPRES", perFaceValues);

            if (data.header.hasPhase(Phase::Water)) {
                checkSize(step.swat, "FLXSATW", perFaceValues);
            }
            else if (!step.swat.empty()) {
                OPM_THROW(std::invalid_argument, "Water saturation values provided, but water is not active");
            }

            if (data.header.hasPhase(Phase::Gas)) {
                checkSize(step.sgas, "FLXSATG", perFaceValues);
            }
            else if (!step.sgas.empty()) {
                OPM_THROW(std::invalid_argument, "Gas saturation values provided, but gas is not active");
            }

            if (!step.rs.empty()) {
                checkSize(step.rs, "FLXRS", perFaceValues);
            }

            if (!step.rv.empty()) {
                checkSize(step.rv, "FLXRV", perFaceValues);
            }

            if (!step.temperature.empty()) {
                checkSize(step.temperature, "FLXTEMP", perFaceValues);
            }

            if (!step.relPerm.empty()) {
                checkSize(step.relPerm, "FLXKR", perFacePhaseValues);
            }

            if (!step.capPressure.empty()) {
                checkSize(step.capPressure, "FLXPC", perFacePhaseValues);
            }
        }
    }

    if (data.header.numSummaryKeys != static_cast<int>(data.summaryKeys.size())) {
        OPM_THROW(std::invalid_argument,
                  fmt::format("Header numSummaryKeys {} does not match summary key count {}",
                              data.header.numSummaryKeys, data.summaryKeys.size()));
    }
}

void FluxFile::validateSamplesForWrite(const Data& data,
                                       const std::vector<SummarySample>& samples,
                                       const double previousTime)
{
    if (!samples.empty() && data.summaryKeys.empty()) {
        OPM_THROW(std::invalid_argument,
                  "Summary samples provided, but no summary keys are defined");
    }

    auto previous = previousTime;
    for (const auto& sample : samples) {
        if (sample.values.size() != data.summaryKeys.size()) {
            OPM_THROW(std::invalid_argument,
                      fmt::format("Each summary sample must contain {} values, got {}",
                                  data.summaryKeys.size(), sample.values.size()));
        }

        if (sample.time < previous) {
            OPM_THROW(std::invalid_argument,
                      fmt::format("Summary sample times must be non-decreasing, "
                                  "got {} after {}", sample.time, previous));
        }

        previous = sample.time;
    }
}

void FluxFile::validateAfterRead(const Data& data)
{
    if (data.header.numCells != static_cast<int>(data.localToGlobal.size())) {
        OPM_THROW(std::runtime_error,
                  fmt::format("LOC2GLOB size {} does not match FLUXHEAD numCells {}",
                              data.localToGlobal.size(), data.header.numCells));
    }

    if (data.header.numBoundaryFaces != static_cast<int>(data.boundaryFaces.size())) {
        OPM_THROW(std::runtime_error,
                  fmt::format("Boundary face count {} does not match FLUXHEAD numBoundaryFaces {}",
                              data.boundaryFaces.size(), data.header.numBoundaryFaces));
    }

    if (data.header.numPhases != std::popcount(static_cast<unsigned int>(data.header.phaseMask))) {
        OPM_THROW(std::runtime_error,
                  fmt::format("FLUXHEAD numPhases {} does not match phaseMask 0x{:x}",
                              data.header.numPhases, data.header.phaseMask));
    }

    if (data.header.numSummaryKeys != static_cast<int>(data.summaryKeys.size())) {
        OPM_THROW(std::runtime_error,
                  fmt::format("Summary key count {} does not match FLUXHEAD numSummaryKeys {}",
                              data.summaryKeys.size(), data.header.numSummaryKeys));
    }
}

} // namespace Opm::EclIO