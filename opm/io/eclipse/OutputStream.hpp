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

#ifndef OPM_IO_OUTPUTSTREAM_HPP_INCLUDED
#define OPM_IO_OUTPUTSTREAM_HPP_INCLUDED

#include <ios>
#include <memory>
#include <string>
#include <vector>

namespace Opm { namespace ecl {

    class EclOutput;

}} // namespace Opm::ecl

namespace Opm { namespace ecl { namespace OutputStream {

    struct Formatted { bool set; };
    struct Unified   { bool set; };

    /// Abstract representation of an ECLIPSE-style result set.
    struct ResultSet
    {
        /// Output directory.  Commonly "." or location of run's .DATA file.
        std::string outputDir;

        /// Base name of simulation run.
        std::string baseName;
    };

    /// File manager for restart output streams.
    class Restart
    {
    public:
        /// Constructor.
        ///
        /// \param[in] rset Output directory and base name of output stream.
        ///
        /// \param[in] fmt Whether or not to create formatted output files.
        ///
        /// \param[in] unif Whether or not to create unified output files.
        explicit Restart(ResultSet        rset,
                         const Formatted& fmt,
                         const Unified&   unif);

        ~Restart();

        Restart(const Restart& rhs) = delete;
        Restart(Restart&& rhs);

        Restart& operator=(const Restart& rhs) = delete;
        Restart& operator=(Restart&& rhs);

        /// Opens file stream pertaining to restart of particular report
        /// step and also outputs a SEQNUM record in the case of a unified
        /// output stream.
        ///
        /// Must be called before accessing the stream object through the
        /// stream() member function.
        ///
        /// \param[in] seqnum Sequence number of new report.  One-based
        ///    report step ID.
        void prepareStep(const int seqnum);

        const ResultSet& resultSetDescriptor() const
        {
            return this->rset_;
        }

        bool formatted() const
        {
            return this->formatted_;
        }

        bool unified() const
        {
            return this->unified_;
        }

        /// Generate a message string (keyword type 'MESS') in underlying
        /// output stream.
        ///
        /// Must not be called prior to \c prepareStep
        ///
        /// \param[in] msg Message string (e.g., "STARTSOL").
        void message(const std::string& msg);

        /// Write integer data to underlying output stream.
        ///
        /// Must not be called prior to \c prepareStep
        ///
        /// \param[in] kw Name of output vector (keyword).
        ///
        /// \param[in] data Output values.
        void write(const std::string&      kw,
                   const std::vector<int>& data);

        /// Write boolean data to underlying output stream.
        ///
        /// Must not be called prior to \c prepareStep
        ///
        /// \param[in] kw Name of output vector (keyword).
        ///
        /// \param[in] data Output values.
        void write(const std::string&       kw,
                   const std::vector<bool>& data);

        /// Write single precision floating point data to underlying
        /// output stream.
        ///
        /// Must not be called prior to \c prepareStep
        ///
        /// \param[in] kw Name of output vector (keyword).
        ///
        /// \param[in] data Output values.
        void write(const std::string&        kw,
                   const std::vector<float>& data);

        /// Write double precision floating point data to underlying
        /// output stream.
        ///
        /// Must not be called prior to \c prepareStep
        ///
        /// \param[in] kw Name of output vector (keyword).
        ///
        /// \param[in] data Output values.
        void write(const std::string&         kw,
                   const std::vector<double>& data);

        /// Write unpadded string data to underlying output stream.
        ///
        /// Must not be called prior to \c prepareStep
        ///
        /// \param[in] kw Name of output vector (keyword).
        ///
        /// \param[in] data Output values.
        void write(const std::string&              kw,
                   const std::vector<std::string>& data);

    private:
        ResultSet rset_;
        bool      formatted_;
        bool      unified_;

        /// Restart output stream.
        std::unique_ptr<EclOutput> stream_;

        /// Open unified output file and place stream's output indicator
        /// in appropriate location.
        ///
        /// Writes to \c stream_.
        ///
        /// \param[in] fname Filename of output stream.
        ///
        /// \param[in] seqnum Sequence number of new report.  One-based
        ///    report step ID.
        void openUnified(const std::string& fname,
                         const int          seqnum);

        /// Open new output stream.
        ///
        /// Handles the case of separate output files or unified output file
        /// that does not already exist.  Writes to \c stream_.
        ///
        /// \param[in] fname Filename of new output stream.
        void openNew(const std::string& fname);

        /// Open existing output file and place stream's output indicator
        /// in appropriate location.
        ///
        /// Writes to \c stream_.
        ///
        /// \param[in] fname Filename of output stream.
        ///
        /// \param[in] writePos Position at which to place stream's output
        ///    indicator.  Use \code streampos{ streamoff{-1} } \endcode to
        ///    place output indicator at end of file (i.e, simple append).
        void openExisting(const std::string&   fname,
                          const std::streampos writePos);

        /// Access writable output stream.
        ///
        /// Must not be called prior to \c prepareStep.
        EclOutput& stream();

        /// Implementation function for public \c write overload set.
        template <typename T>
        void writeImpl(const std::string&    kw,
                       const std::vector<T>& data);
    };

    /// Derive filename corresponding to output stream of particular result
    /// set, with user-specified file extension.
    ///
    /// Low-level string concatenation routine that does not know specific
    /// relations between base names and file extensions.  Handles details
    /// of base name ending in a period (full stop) or having a name that
    /// might otherwise appear to contain a file extension (e.g., CASE.01).
    ///
    /// \param[in] rsetDescriptor Output directory and base name of result set.
    ///
    /// \param[in] ext Filename extension.
    ///
    /// \return outputDir/baseName.ext
    std::string outputFileName(const ResultSet&   rsetDescriptor,
                               const std::string& ext);

    /// Derive filename of restart output stream.
    ///
    /// Knows the rules for formatted vs. unformatted, and separate vs.
    /// unified output files.
    ///
    /// \param[in] rst Previously configured restart stream.
    ///
    /// \param[in] seqnum Sequence number (1-based report step ID).
    ///
    /// \return Filename corresponding to stream's configuration.
    std::string outputFileName(const Restart& rst,
                               const int      seqnum);

}}} // namespace Opm::ecl::OutputStream

#endif //  OPM_IO_OUTPUTSTREAM_HPP_INCLUDED
