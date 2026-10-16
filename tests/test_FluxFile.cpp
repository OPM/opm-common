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

#include "config.h"

#define BOOST_TEST_MODULE Test FluxFile
#include <boost/test/unit_test.hpp>

#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/EclOutput.hpp>
#include <opm/io/eclipse/FluxFile.hpp>

#include <filesystem>
#include <stdexcept>
#include <cmath>
#include <string>
#include <vector>

#include "WorkArea.hpp"

namespace {

Opm::EclIO::FluxFile::Data sampleData()
{
    using FluxFile = Opm::EclIO::FluxFile;

    FluxFile::Data data;
    data.header.parentNx = 20;
    data.header.parentNy = 30;
    data.header.parentNz = 10;
    data.header.boxI1 = 2;
    data.header.boxJ1 = 9;
    data.header.boxK1 = 1;
    data.header.boxNx = 19;
    data.header.boxNy = 10;
    data.header.boxNz = 10;
    data.header.numCells = 4;
    data.header.numBoundaryFaces = 2;
    data.header.numReportSteps = 2;
    data.header.numPhases = 3;
    data.header.phaseMask = static_cast<int>(FluxFile::Phase::Oil)
                          | static_cast<int>(FluxFile::Phase::Water)
                          | static_cast<int>(FluxFile::Phase::Gas);
    data.header.hasTemperature = true;
    data.header.mode = FluxFile::Mode::Both;
    data.header.sampling = FluxFile::Sampling::Averaged;
    data.header.numSummaryKeys = 2;
    data.header.numSummarySamples = 3;
    data.header.summaryPerTimestep = true;
    data.header.summaryMinSampleInterval = 86400.0;
    // The two records below carry distinct report step numbers, and this flag
    // is now recovered from that rather than read from the file.
    data.header.boundaryPerTimestep = false;
    data.header.boundaryMinSampleInterval = 43200.0;

    data.names = {"BASE", "REGION_2", "METRIC"};
    data.localToGlobal = {248, 249, 268, -1};
    data.boundaryFaces = {
        // The last three members are the exterior cell's PVT region, the depth
        // of its centre and its equilibration region. The second face
        // deliberately leaves all three unrecorded, which is how a file
        // written before they were stored reads back, and has to survive the
        // round trip as such.
        {0, 0, 247, 1.5, 0, 1606.802, 2},
        {2, 3, 288, 2.5},
    };

    // A 3x3 table over three equilibration regions. Deliberately asymmetric:
    // THPRES may be given as irreversible, and reading the pair the wrong way
    // round then silently picks the other direction's threshold.
    data.thresholdPressure = {
        0.0,     53836.49, 0.0,
        0.0,     0.0,      81483.75,
        12345.0, 0.0,      0.0,
    };
    data.summaryKeys = {"FOPR", "GGPR"};

    data.reportSteps.resize(2);
    {
        auto& step = data.reportSteps[0];
        step.reportStep = 0;
        step.simStep = 0;
        step.startTime = 0.0;
        step.stepLength = 0.5;
        step.massRates = {10.0, 11.0, 12.0, 20.0, 21.0, 22.0};
        step.pressures = {200.0, 210.0};
        step.swat = {0.15, 0.25};
        step.sgas = {0.05, 0.15};
        step.rs = {100.0, 110.0};
        step.rv = {5.0, 6.0};
        step.temperature = {330.0, 331.0};
    }
    {
        auto& step = data.reportSteps[1];
        step.reportStep = 1;
        step.simStep = 3;
        step.startTime = 0.5;
        step.stepLength = 28.0;
        step.massRates = {13.0, 14.0, 15.0, 23.0, 24.0, 25.0};
        step.pressures = {220.0, 230.0};
        step.swat = {0.16, 0.26};
        step.sgas = {0.06, 0.16};
        step.rs = {120.0, 130.0};
        step.rv = {7.0, 8.0};
        step.temperature = {332.0, 333.0};
    }

    // Summary samples are deliberately independent of the report-step
    // sequence: three samples spanning two report steps.
    data.summarySamples = {
        {0.5, {1000.0, 2000.0}},
        {10.0, {1100.0, 2100.0}},
        {28.5, {1200.0, 2200.0}},
    };

    return data;
}

// Append everything in one go. There is no whole-file write: a FLUX file is
// only ever produced by appending, so even a caller holding the complete data
// goes through the writer.
void writeAll(const std::string& filename,
              const bool formatted,
              const Opm::EclIO::FluxFile::Data& data)
{
    Opm::EclIO::FluxFile::Writer writer(filename, formatted, data);
    writer.appendRecords(data.reportSteps);
    writer.appendSummarySamples(data.summarySamples);
    writer.close();
}

void expectRoundTrip(const std::string& filename, const bool formatted)
{
    const auto written = sampleData();
    writeAll(filename, formatted, written);
    const auto readBack = Opm::EclIO::FluxFile::read(filename);
    BOOST_CHECK(readBack == written);
}

Opm::EclIO::FluxFile::Data gasOilData()
{
    using FluxFile = Opm::EclIO::FluxFile;

    auto data = sampleData();
    data.header.numPhases = 2;
    data.header.phaseMask = static_cast<int>(FluxFile::Phase::Oil)
                          | static_cast<int>(FluxFile::Phase::Gas);

    for (auto& step : data.reportSteps) {
        step.massRates = {
            step.massRates[0], step.massRates[2],
            step.massRates[3], step.massRates[5],
        };
        step.swat.clear();
    }

    return data;
}

} // namespace

BOOST_AUTO_TEST_CASE(BinaryRoundTrip)
{
    WorkArea work;
    expectRoundTrip("SAMPLE.FLUX", false);
}

BOOST_AUTO_TEST_CASE(FormattedRoundTrip)
{
    WorkArea work;
    expectRoundTrip("SAMPLE.FFLUX", true);
}

BOOST_AUTO_TEST_CASE(ExteriorDepthSurvivesTheRoundTrip)
{
    // A reduced run imposes the recorded pressures at the boundary face, while
    // they were measured at the centre of the cell on the far side. It carries
    // them from the one to the other using this depth, so losing it in the file
    // means imposing the exterior cell's pressure at a depth where the exterior
    // cell does not have it -- which in a dipping layer is metres of head.
    WorkArea work;

    const auto written = sampleData();
    writeAll("DEPTH.FLUX", false, written);
    const auto readBack = Opm::EclIO::FluxFile::read("DEPTH.FLUX");

    BOOST_REQUIRE_EQUAL(readBack.boundaryFaces.size(), written.boundaryFaces.size());

    BOOST_CHECK_CLOSE(readBack.boundaryFaces[0].exteriorDepth, 1606.802, 1.0e-10);

    // And a face whose depth the producer never recorded still reads back as
    // unrecorded rather than as a depth of zero, which a consumer would
    // otherwise treat as a real datum.
    BOOST_CHECK(std::isnan(readBack.boundaryFaces[1].exteriorDepth));
}

BOOST_AUTO_TEST_CASE(ThresholdPressuresSurviveTheRoundTrip)
{
    // A defaulted THPRES entry is the largest initial potential difference
    // along a whole region boundary, so a sector holding part of that boundary
    // cannot arrive at the same number and has to be handed the producer's.
    // The exterior cell's region is what pairs with the interior one to pick
    // an entry out of the table, and without it the table cannot be used.
    WorkArea work;

    const auto written = sampleData();
    writeAll("THPRES.FLUX", false, written);
    const auto readBack = Opm::EclIO::FluxFile::read("THPRES.FLUX");

    BOOST_CHECK_EQUAL_COLLECTIONS(readBack.thresholdPressure.begin(),
                                  readBack.thresholdPressure.end(),
                                  written.thresholdPressure.begin(),
                                  written.thresholdPressure.end());

    BOOST_REQUIRE_EQUAL(readBack.boundaryFaces.size(), written.boundaryFaces.size());
    BOOST_CHECK_EQUAL(readBack.boundaryFaces[0].exteriorEquilRegion, 2);

    // A face whose region the producer never recorded reads back as unrecorded
    // rather than as region zero, which is a real region and would pick a real
    // entry out of the table.
    BOOST_CHECK_EQUAL(readBack.boundaryFaces[1].exteriorEquilRegion, -1);
}

BOOST_AUTO_TEST_CASE(OnlyPresentPhasesAreWritten)
{
    WorkArea work;

    const auto written = gasOilData();
    writeAll("GASOIL.FLUX", false, written);

    Opm::EclIO::EclFile file("GASOIL.FLUX", Opm::EclIO::EclFile::Formatted{false}, true);
    BOOST_CHECK(!file.hasKey("FLXSATW"));
    BOOST_CHECK(file.hasKey("FLXSATG"));
    BOOST_CHECK(file.hasKey("FLXRS"));
    BOOST_CHECK(file.hasKey("FLXRV"));

    const auto readBack = Opm::EclIO::FluxFile::read("GASOIL.FLUX");
    BOOST_CHECK(readBack == written);
}

BOOST_AUTO_TEST_CASE(RejectsUnsupportedVersion)
{
    WorkArea work;

    Opm::EclIO::EclOutput output("BADVERSION.FLUX", false);
    output.write("FLUXHEAD", std::vector<int>{99, 20, 30, 10, 2, 9, 1, 19, 10, 10, 4, 2, 1, 3, 0, 1, 1, 7, 0, 0, 0, 0});
    output.write("FLUXNAMS", std::vector<std::string>{"BASE", "REGION_2"}, 32);
    output.write("FLUXNCNT", std::vector<int>{2, 0});
    output.write("LOC2GLOB", std::vector<int>{1, 2, 3, 4});
    output.write("FACECELL", std::vector<int>{0, 1});
    output.write("FACEDIR", std::vector<int>{0, 1});
    output.write("FACEGLNB", std::vector<int>{10, 11});
    output.write("FLUXTRAN", std::vector<double>{1.0, 2.0});
    output.write("FLXSTEP", std::vector<int>{0});
    output.write("FLXSIM", std::vector<int>{0});
    output.write("FLXTIME", std::vector<double>{0.0});
    output.write("FLXDT", std::vector<double>{1.0});
    output.write("FLXMASS", std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0});

    BOOST_CHECK_THROW(Opm::EclIO::FluxFile::read("BADVERSION.FLUX"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(RejectsMissingRequiredArray)
{
    WorkArea work;

    Opm::EclIO::EclOutput output("MISSING.FLUX", false);
    output.write("FLUXHEAD", std::vector<int>{7, 20, 30, 10, 2, 9, 1, 19, 10, 10, 4, 2, 1, 3, 0, 1, 1, 7, 0, 0, 0, 0});
    output.write("FLUXNAMS", std::vector<std::string>{"BASE", "REGION_2"}, 32);
    output.write("FLUXNCNT", std::vector<int>{2, 0});
    output.write("LOC2GLOB", std::vector<int>{1, 2, 3, 4});
    output.write("FACECELL", std::vector<int>{0, 1});
    output.write("FACEDIR", std::vector<int>{0, 1});
    output.write("FLUXTRAN", std::vector<double>{1.0, 2.0});
    output.write("FLXSTEP", std::vector<int>{0});
    output.write("FLXSIM", std::vector<int>{0});
    output.write("FLXTIME", std::vector<double>{0.0});
    output.write("FLXDT", std::vector<double>{1.0});
    output.write("FLXMASS", std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0});

    BOOST_CHECK_THROW(Opm::EclIO::FluxFile::read("MISSING.FLUX"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(SummarySamplesAreIndependentOfReportSteps)
{
    WorkArea work;

    const auto written = sampleData();

    // Three summary samples against two report steps: the sample series is
    // sampled at sub-report-step resolution and must survive a round trip
    // without being forced onto the report-step grid.
    BOOST_REQUIRE_EQUAL(written.summarySamples.size(), 3U);
    BOOST_REQUIRE_EQUAL(written.reportSteps.size(), 2U);

    writeAll("SMRY.FLUX", false, written);

    Opm::EclIO::EclFile file("SMRY.FLUX", Opm::EclIO::EclFile::Formatted{false}, true);
    BOOST_CHECK(file.hasKey("SMRYTIME"));
    BOOST_CHECK(file.hasKey("SMRYVALS"));
    BOOST_CHECK(file.hasKey("SMRYMINT"));

    const auto readBack = Opm::EclIO::FluxFile::read("SMRY.FLUX");
    BOOST_CHECK(readBack == written);

    BOOST_REQUIRE_EQUAL(readBack.summarySamples.size(), 3U);
    BOOST_CHECK_CLOSE(readBack.summarySamples[1].time, 10.0, 1e-12);
    BOOST_REQUIRE_EQUAL(readBack.summarySamples[1].values.size(), 2U);
    BOOST_CHECK_CLOSE(readBack.summarySamples[1].values[0], 1100.0, 1e-12);
    BOOST_CHECK_CLOSE(readBack.summarySamples[1].values[1], 2100.0, 1e-12);
    BOOST_CHECK_CLOSE(readBack.header.summaryMinSampleInterval, 86400.0, 1e-12);
    BOOST_CHECK(readBack.header.summaryPerTimestep);

    // The boundary cadence metadata travels independently of the summary data.
    BOOST_CHECK(!readBack.header.boundaryPerTimestep);
    BOOST_CHECK_CLOSE(readBack.header.boundaryMinSampleInterval, 43200.0, 1e-12);
}

BOOST_AUTO_TEST_CASE(RecordsSharingAReportStepSetTheBoundaryCadence)
{
    WorkArea work;

    // Two records under the same report step number is what sub-report-step
    // boundary output looks like, and the flag is recovered from that rather
    // than stored: the header is written before any record exists.
    auto written = sampleData();
    written.reportSteps[1].reportStep = written.reportSteps[0].reportStep;
    written.header.boundaryPerTimestep = true;

    writeAll("PERSTEP.FLUX", false, written);

    const auto readBack = Opm::EclIO::FluxFile::read("PERSTEP.FLUX");
    BOOST_CHECK(readBack.header.boundaryPerTimestep);
    BOOST_CHECK(readBack == written);
}

BOOST_AUTO_TEST_CASE(AppendingInChunksMatchesASingleBlock)
{
    WorkArea work;

    const auto expected = sampleData();

    writeAll("ONEBLOCK.FLUX", false, expected);

    // The same records, appended one block at a time, the way a running
    // simulation produces them.
    {
        Opm::EclIO::FluxFile::Writer writer("CHUNKED.FLUX", false, expected);
        for (const auto& step : expected.reportSteps) {
            writer.appendRecords({step});
        }
        for (const auto& sample : expected.summarySamples) {
            writer.appendSummarySamples({sample});
        }
        writer.close();
    }

    const auto single = Opm::EclIO::FluxFile::read("ONEBLOCK.FLUX");
    const auto chunked = Opm::EclIO::FluxFile::read("CHUNKED.FLUX");

    BOOST_CHECK(single == expected);
    BOOST_CHECK(chunked == expected);
    BOOST_CHECK(chunked == single);
}

BOOST_AUTO_TEST_CASE(ATruncatedFinalBlockIsDiscardedRatherThanFatal)
{
    WorkArea work;

    const auto expected = sampleData();

    {
        Opm::EclIO::FluxFile::Writer writer("TORN.FLUX", false, expected);
        writer.appendRecords({expected.reportSteps[0]});
        writer.appendRecords({expected.reportSteps[1]});
        writer.close();
    }

    // Chop the tail, as a run killed part way through a write would leave it.
    const auto full = std::filesystem::file_size("TORN.FLUX");
    BOOST_REQUIRE(full > 64U);
    std::filesystem::resize_file("TORN.FLUX", full - 40U);

    const auto readBack = Opm::EclIO::FluxFile::read("TORN.FLUX");

    // The first block survives intact; the damaged one is gone.
    BOOST_REQUIRE_EQUAL(readBack.reportSteps.size(), 1U);
    BOOST_CHECK(readBack.reportSteps[0] == expected.reportSteps[0]);
    BOOST_CHECK_EQUAL(readBack.header.numReportSteps, 1);
}

BOOST_AUTO_TEST_CASE(RoundTripWithoutSummarySamples)
{
    WorkArea work;

    auto written = sampleData();
    written.summaryKeys.clear();
    written.summarySamples.clear();
    written.header.numSummaryKeys = 0;
    written.header.numSummarySamples = 0;
    written.header.summaryPerTimestep = false;
    written.header.summaryMinSampleInterval = 0.0;

    writeAll("NOSMRY.FLUX", false, written);

    Opm::EclIO::EclFile file("NOSMRY.FLUX", Opm::EclIO::EclFile::Formatted{false}, true);
    BOOST_CHECK(!file.hasKey("SMRYTIME"));
    BOOST_CHECK(!file.hasKey("SMRYVALS"));

    // The boundary interval is written even when there is no summary payload.
    BOOST_CHECK(file.hasKey("FLXMINT"));

    const auto readBack = Opm::EclIO::FluxFile::read("NOSMRY.FLUX");
    BOOST_CHECK(readBack == written);
    BOOST_CHECK_CLOSE(readBack.header.boundaryMinSampleInterval, 43200.0, 1e-12);
}

BOOST_AUTO_TEST_CASE(RejectsInconsistentSummarySampleWidth)
{
    WorkArea work;

    auto data = sampleData();
    data.summarySamples[1].values.pop_back();

    BOOST_CHECK_THROW(writeAll("BADWIDTH.FLUX", false, data),
                      std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(RejectsDecreasingSummarySampleTimes)
{
    WorkArea work;

    auto data = sampleData();
    data.summarySamples[2].time = data.summarySamples[1].time - 1.0;

    BOOST_CHECK_THROW(writeAll("BADTIME.FLUX", false, data),
                      std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(RecordAndSampleCountsAreDerivedNotTrusted)
{
    WorkArea work;

    // The header is written before a single record exists, so its counts
    // cannot be right and are not read back. Whatever nonsense is in the
    // struct on the way in, what comes out is what the file actually holds.
    auto data = sampleData();
    data.header.numReportSteps = 99;
    data.header.numSummarySamples = 99;

    writeAll("COUNTS.FLUX", false, data);

    const auto readBack = Opm::EclIO::FluxFile::read("COUNTS.FLUX");
    BOOST_CHECK_EQUAL(readBack.header.numReportSteps, 2);
    BOOST_CHECK_EQUAL(readBack.header.numSummarySamples, 3);
    BOOST_CHECK_EQUAL(readBack.reportSteps.size(), 2U);
    BOOST_CHECK_EQUAL(readBack.summarySamples.size(), 3U);
}