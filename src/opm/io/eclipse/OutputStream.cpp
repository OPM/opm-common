/*
  Copyright (c) 2019 Equinor ASA

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

#include <opm/io/eclipse/OutputStream.hpp>

#include <opm/io/eclipse/EclOutput.hpp>
#include <opm/io/eclipse/ERst.hpp>

#include <exception>
#include <fstream>
#include <iomanip>
#include <ios>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/filesystem.hpp>

namespace {
    namespace FileExtension
    {
        std::string
        restart(const int  rptStep,
                const bool formatted,
                const bool unified)
        {
            if (unified) {
                return formatted ? "FUNRST" : "UNRST";
            }

            std::ostringstream ext;

            ext << (formatted ? 'F' : 'X')
                << std::setw(4) << std::setfill('0')
                << rptStep;

            return ext.str();
        }
    } // namespace FileExtension

    namespace Open
    {
        namespace Restart
        {
            std::unique_ptr<Opm::ecl::ERst>
            read(const std::string& filename)
            {
                // Bypass some of the internal logic of the ERst constructor.
                //
                // Specifically, the base class EclFile's constructor throws
                // and outputs a highly visible diagnostic message if it is
                // unable to open the file.  The diagnostic message is very
                // confusing to users if they are running a simulation case
                // for the first time and will likely provoke a reaction along
                // the lines of "well of course the restart file doesn't exist".
                {
                    std::ifstream is(filename);

                    if (! is) {
                        // Unable to open (does not exist?).  Return null.
                        return {};
                    }
                }

                // File exists and can (could?) be opened.  Attempt to form
                // an ERst object on top of it.
                return std::unique_ptr<Opm::ecl::ERst> {
                    new Opm::ecl::ERst{filename}
                };
            }

            std::unique_ptr<Opm::ecl::EclOutput>
            writeNew(const std::string& filename,
                     const bool         isFmt)
            {
                return std::unique_ptr<Opm::ecl::EclOutput> {
                    new Opm::ecl::EclOutput {
                        filename, isFmt, std::ios_base::out
                    }
                };
            }

            std::unique_ptr<Opm::ecl::EclOutput>
            writeExisting(const std::string& filename,
                          const bool         isFmt)
            {
                return std::unique_ptr<Opm::ecl::EclOutput> {
                    new Opm::ecl::EclOutput {
                        filename, isFmt, std::ios_base::app
                    }
                };
            }
        } // namespace Restart
    } // namespace Open
} // Anonymous namespace

Opm::ecl::OutputStream::Restart::
Restart(const ResultSet& rset,
        const int        seqnum,
        const Formatted& fmt,
        const Unified&   unif)
{
    const auto ext = FileExtension::
        restart(seqnum, fmt.set, unif.set);

    const auto fname = outputFileName(rset, ext);

    if (unif.set) {
        // Run uses unified restart files.
        this->openUnified(fname, fmt.set, seqnum);

        // Write SEQNUM value to stream to start new output sequence.
        this->stream_->write("SEQNUM", std::vector<int>{ seqnum });
    }
    else {
        // Run uses separate, not unified, restart files.  Create a
        // new output file and open an output stream on it.
        this->openNew(fname, fmt.set);
    }
}

Opm::ecl::OutputStream::Restart::~Restart()
{}

Opm::ecl::OutputStream::Restart::Restart(Restart&& rhs)
    : stream_{ std::move(rhs.stream_) }
{}

Opm::ecl::OutputStream::Restart&
Opm::ecl::OutputStream::Restart::operator=(Restart&& rhs)
{
    this->stream_ = std::move(rhs.stream_);

    return *this;
}

void Opm::ecl::OutputStream::Restart::message(const std::string& msg)
{
    this->stream().message(msg);
}

void
Opm::ecl::OutputStream::Restart::
write(const std::string& kw, const std::vector<int>& data)
{
    this->writeImpl(kw, data);
}

void
Opm::ecl::OutputStream::Restart::
write(const std::string& kw, const std::vector<bool>& data)
{
    this->writeImpl(kw, data);
}

void
Opm::ecl::OutputStream::Restart::
write(const std::string& kw, const std::vector<float>& data)
{
    this->writeImpl(kw, data);
}

void
Opm::ecl::OutputStream::Restart::
write(const std::string& kw, const std::vector<double>& data)
{
    this->writeImpl(kw, data);
}

void
Opm::ecl::OutputStream::Restart::
write(const std::string& kw, const std::vector<std::string>& data)
{
    this->writeImpl(kw, data);
}

void
Opm::ecl::OutputStream::Restart::
openUnified(const std::string& fname,
            const bool         formatted,
            const int          seqnum)
{
    // Determine if we're creating a new output/restart file or
    // if we're opening an existing one, possibly at a specific
    // write position.
    auto rst = Open::Restart::read(fname);

    if (rst == nullptr) {
        // No such unified restart file exists.  Create new file.
        this->openNew(fname, formatted);
    }
    else if (! rst->hasKey("SEQNUM")) {
        // File with correct filename exists but does not appear
        // to be an actual unified restart file.
        throw std::invalid_argument {
            "Purported existing unified restart file '"
            + boost::filesystem::path{fname}.filename().string()
            + "' does not appear to be a unified restart file"
        };
    }
    else {
        // Restart file exists and appears to be a unified restart
        // resource.  Open writable restart stream backed by the
        // specific file.
        this->openExisting(fname, formatted,
                           rst->restartStepWritePosition(seqnum));
    }
}

void
Opm::ecl::OutputStream::Restart::
openNew(const std::string& fname,
        const bool         formatted)
{
    this->stream_ = Open::Restart::writeNew(fname, formatted);
}

void
Opm::ecl::OutputStream::Restart::
openExisting(const std::string&   fname,
             const bool           formatted,
             const std::streampos writePos)
{
    this->stream_ = Open::Restart::writeExisting(fname, formatted);

    if (writePos == std::streampos(-1)) {
        // No specified initial write position.  Typically the case if
        // requested SEQNUM value exceeds all existing SEQNUM values in
        // 'fname'.  This is effectively a simple append operation so
        // no further actions required.
        return;
    }

    // The user specified an initial write position.  Resize existing
    // file (as if by POSIX function ::truncate()) to requested size,
    // and place output position at that position (i.e., new EOF).  This
    // case typically corresponds to reopening a unified restart file at
    // the start of a particular SEQNUM keyword.
    //
    // Note that this intentionally operates on the file/path backing the
    // already opened 'stream_'.  In other words, 'open' followed by
    // resize_file() followed by seekp() is the intended and expected
    // order of operations.

    boost::filesystem::resize_file(fname, writePos);

    if (! this->stream_->ofileH.seekp(0, std::ios_base::end)) {
        throw std::invalid_argument {
            "Unable to Seek to Write Position " +
            std::to_string(writePos) + " of File '"
            + fname + "'"
        };
    }
}

Opm::ecl::EclOutput&
Opm::ecl::OutputStream::Restart::stream()
{
    return *this->stream_;
}

namespace Opm { namespace ecl { namespace OutputStream {

    template <typename T>
    void Restart::writeImpl(const std::string&    kw,
                            const std::vector<T>& data)
    {
        this->stream().write(kw, data);
    }

}}}


std::string
Opm::ecl::OutputStream::outputFileName(const ResultSet&   rsetDescriptor,
                                       const std::string& ext)
{
    namespace fs = boost::filesystem;

    // Allow baseName = "CASE", "CASE.", "CASE.N", or "CASE.N.".
    auto fname = fs::path {
        rsetDescriptor.baseName
        + (rsetDescriptor.baseName.back() == '.' ? "" : ".")
        + "REPLACE"
    }.replace_extension(ext);

    return (fs::path { rsetDescriptor.outputDir } / fname)
        .generic().string();
}
