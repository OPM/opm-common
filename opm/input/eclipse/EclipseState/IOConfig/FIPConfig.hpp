/*
  Copyright 2023 SINTEF Digital

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

#ifndef OPM_FIP_CONFIG_HPP
#define OPM_FIP_CONFIG_HPP

#include <bitset>

namespace Opm {

class Deck;
class DeckKeyword;
class RPTConfig;

//! \brief Class holding FIP configuration from RPTSOL/RPTSCHED keyword.
class FIPConfig {
public:
    //! \brief Enumeration of FIP report outputs.
    enum FIPOutputField {
        FIELD              =  0, //!< Whole field
        FIPNUM             =  1, //!< FIPNUM regions
        FIP                =  2, //!< FIP defined regions
        FOAM_FIELD         =  3, //!< Foam field report
        FOAM_REGION        =  4, //!< Foam region report
        POLYMER_FIELD      =  5, //!< Polymer field report
        POLYMER_REGION     =  6, //!< Polymer region report
        RESV               =  7, //!< RESV report
        SOLVENT_FIELD      =  8, //!< Solvent field report
        SOLVENT_REGION     =  9, //!< Solvent region report
        TEMPERATURE_FIELD  = 10, //!< Temperature field report
        TEMPERATURE_REGION = 11, //!< Temperature region report
        SURF_FIELD         = 12, //!< Surfacant field report
        SURF_REGION        = 13, //!< Surfacant region report
        TRACER_FIELD       = 14, //!< Tracer field report
        TRACER_REGION      = 15, //!< Tracer region report
        VE                 = 16, //!< VE (oil, water, gas) zone report
        NUM_FIP_REPORT     = 17, //!< Number of configuration flags
    };

    //! \brief Default constructor.
    FIPConfig() = default;

    //! \brief Construct from RPTSOL keyword if deck holds one.
    explicit FIPConfig(const Deck& deck);

    //! \brief Construct from given keyword (RPTSOL or RPTSCHED).
    explicit FIPConfig(const DeckKeyword& keyword);

    //! \brief Construct from given RTPConfig.
    explicit FIPConfig(const RPTConfig& rptConfig);

    //! \brief Returns a test object used for serialization tests.
    static FIPConfig serializationTestObject();

    //! \brief (De-)serialization handler.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_flags);
    }

    //! \brief Query if FIP output is enabled for a given field.
    bool output(FIPOutputField field) const;

    //! \brief Comparison operator.
    bool operator==(const FIPConfig& rhs) const;

private:
    //! \brief Initialize flags based on mnemonics in a RPTConfig instance.
    void parseRPT(const RPTConfig& rptConfig);

    std::bitset<NUM_FIP_REPORT> m_flags = {}; //!< Bitset holding enable status for fields
};

} //namespace Opm

#endif
