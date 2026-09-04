/*
  Copyright 2017 Statoil ASA.

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

#include <opm/input/eclipse/Deck/DeckOutput.hpp>
#include <opm/input/eclipse/Deck/UDAValue.hpp>
#include <opm/input/eclipse/Utility/Typetools.hpp>

#include <ostream>
#include <sstream>

namespace Opm {

    DeckOutput::DeckOutput( std::ostream& s, int precision) :
        os( s ),
        default_count( 0 ),
        row_count( 0 ),
        current_width( 0 ),
        record_on( false ),
        org_precision( os.precision(precision) ),
        split_line( false )
    {}


    DeckOutput::~DeckOutput() {
        this->set_precision(this->org_precision);
    }


    void DeckOutput::set_precision(int precision) {
        this->os.precision(precision);
    }


    void DeckOutput::endl() {
        this->os << std::endl;
        this->current_width = 0;
    }

    void DeckOutput::write_string(const std::string& s) {
        this->os << s;
        this->current_width += s.size();
    }


    template <typename T>
    void DeckOutput::write( const T& value ) {
        if (default_count > 0) {
            this->write_token(std::to_string(default_count) + "*");
            default_count = 0;
        }

        this->write_token(this->format_value(value));
    }

    template <>
    std::string DeckOutput::format_value( const std::string& value ) {
        return "'" + value + "'";
    }

    template <>
    std::string DeckOutput::format_value( const RawString& value ) {
        return { value };
    }

    template <>
    std::string DeckOutput::format_value( const int& value ) {
        return std::to_string( value );
    }

    template <>
    std::string DeckOutput::format_value( const double& value ) {
        std::ostringstream ss;
        ss.flags( this->os.flags() );
        ss.precision( this->os.precision() );
        ss << value;
        return ss.str();
    }

    template <>
    std::string DeckOutput::format_value( const UDAValue& value ) {
        if (value.is<double>())
            return this->format_value( value.get<double>() );
        else
            return this->format_value( value.get<std::string>() );
    }

    void DeckOutput::write_token( const std::string& token ) {
        this->write_sep( token.size() );
        this->os << token;
        this->current_width += token.size();
        this->row_count++;
    }

    void DeckOutput::stash_default( ) {
        this->default_count++;
    }


    void DeckOutput::start_keyword(const std::string& kw, bool split_line_arg) {
        this->os << kw << std::endl;
        this->current_width = 0;
        this->split_line = split_line_arg;
    }


    void DeckOutput::end_keyword(bool add_slash) {
        if (add_slash) {
            this->os << "/" << std::endl;
            this->current_width = 0;
        }
    }


    void DeckOutput::write_sep( std::size_t next_token_width ) {
        if (record_on && (row_count > 0)) {
            bool split = this->split_line && ((row_count % this->fmt.columns) == 0);

            // Break the line before it grows past the maximum width.  Two characters
            // are reserved for the " /" which terminates the record.
            if (this->fmt.max_line_width > 0) {
                const auto width = this->current_width + this->fmt.item_sep.size()
                    + next_token_width + 2;

                split = split || (width > this->fmt.max_line_width);
            }

            if (split)
                split_record();
        }

        if (row_count > 0) {
            os << this->fmt.item_sep;
            this->current_width += this->fmt.item_sep.size();
        } else if (record_on) {
            os << this->fmt.record_indent;
            this->current_width += this->fmt.record_indent.size();
        }
    }

    void DeckOutput::start_record( ) {
        this->default_count = 0;
        this->row_count = 0;
        this->record_on = true;
    }


    void DeckOutput::split_record() {
        this->os << std::endl;
        this->row_count = 0;
        this->current_width = 0;
    }


    void DeckOutput::end_record( ) {
        this->os << " /" << std::endl;
        this->current_width = 0;
        this->record_on = false;
    }


    template void DeckOutput::write( const int& value);
    template void DeckOutput::write( const double& value);
    template void DeckOutput::write( const std::string& value);
    template void DeckOutput::write( const RawString& value);
    template void DeckOutput::write( const UDAValue& value);
}
