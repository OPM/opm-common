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

#include <opm/input/eclipse/EclipseState/InitConfig/InitConfig.hpp>

#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/EclipseState/InitConfig/Equil.hpp>
#include <opm/input/eclipse/EclipseState/InitConfig/FoamConfig.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/E.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/F.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/N.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/R.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/S.hpp>

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

    Opm::Equil equils(const Opm::Deck& deck)
    {
        if (! deck.hasKeyword<Opm::ParserKeywords::EQUIL>()) {
            return {};
        }

        return Opm::Equil {
            deck.get<Opm::ParserKeywords::EQUIL>().back()
        };
    }

    Opm::DeckKeyword stressequils(const Opm::Deck& deck)
    {
        if (! deck.hasKeyword<Opm::ParserKeywords::STREQUIL>()) {
            return {};
        }

        return deck.get<Opm::ParserKeywords::STREQUIL>().back();
    }

} // Anonymous namespace

namespace Opm {

    InitConfig::InitConfig(const Deck& deck)
        : equil        { equils(deck) }
        , stress_equil { stressequils(deck) }
        , foamconfig   { deck }
        , m_filleps    { PROPSSection{deck}.hasKeyword(ParserKeywords::FILLEPS::keywordName) }
        , m_gravity    { ! deck.hasKeyword<ParserKeywords::NOGRAV>() }
    {
        this->parseRestartKeyword(deck);
    }

    InitConfig InitConfig::serializationTestObject()
    {
        InitConfig result;
        result.equil = Equil::serializationTestObject();
        result.stress_equil = StressEquil::serializationTestObject();
        result.foamconfig = FoamConfig::serializationTestObject();
        result.m_filleps = true;
        result.m_gravity = false;
        result.m_restartRequested = true;
        result.m_restartStep = 20;
        result.m_restartRootName = "test1";
        result.m_restartRootNameInput = "test2";

        return result;
    }

    void InitConfig::setRestart(const std::string& root, const int step)
    {
        this->m_restartRequested = true;
        this->m_restartStep = step;
        this->m_restartRootName = root;
    }

    bool InitConfig::restartRequested() const
    {
        return m_restartRequested;
    }

    int InitConfig::getRestartStep() const
    {
        return m_restartStep;
    }

    const std::string& InitConfig::getRestartRootName() const
    {
        return m_restartRootName;
    }

    const std::string& InitConfig::getRestartRootNameInput() const
    {
        return m_restartRootNameInput;
    }

    bool InitConfig::hasEquil() const
    {
        return !this->equil.empty();
    }

    const Equil& InitConfig::getEquil() const
    {
        if (!this->hasEquil()) {
            throw std::runtime_error {
                "Error: No 'EQUIL' present"
            };
        }

        return this->equil;
    }

    bool InitConfig::hasStressEquil() const
    {
        return !this->stress_equil.empty();
    }

    const StressEquil& InitConfig::getStressEquil() const
    {
        if (! this->hasStressEquil()) {
            throw std::runtime_error {
                "Error: No 'STREQUIL' present"
            };
        }

        return this->stress_equil;
    }

    bool InitConfig::hasFoamConfig() const
    {
        // return !this->foamconfig.empty();
        return true;
    }

    const FoamConfig& InitConfig::getFoamConfig() const
    {
        if (! this->hasFoamConfig()) {
            throw std::runtime_error {
                "Error: No foam model configuration keywords present"
            };
        }

        return this->foamconfig;
    }

    bool InitConfig::operator==(const InitConfig& data) const
    {
        return (this->equil == data.equil)
            && (this->stress_equil == data.stress_equil)
            && (this->foamconfig == data.foamconfig)
            && (this->m_filleps == data.m_filleps)
            && (this->m_gravity == data.m_gravity)
            && (this->m_restartRequested == data.m_restartRequested)
            && (this->m_restartStep == data.m_restartStep)
            && (this->m_restartRootName == data.m_restartRootName)
            && (this->m_restartRootNameInput == data.m_restartRootNameInput)
            ;
    }

    bool InitConfig::rst_cmp(const InitConfig& full_config,
                             const InitConfig& rst_config)
    {
        return (full_config.foamconfig == rst_config.foamconfig)
            && (full_config.m_filleps == rst_config.m_filleps)
            && (full_config.m_gravity == rst_config.m_gravity)
            ;
    }

    // -----------------------------------------------------------------------
    // Private member functions of class InitConfig below separator
    // -----------------------------------------------------------------------

    void InitConfig::parseRestartKeyword(const Deck& deck)
    {
        if (! deck.hasKeyword<ParserKeywords::RESTART>()) {
            if (deck.hasKeyword<ParserKeywords::SKIPREST>()) {
                std::cout << "Deck has SKIPREST, but no RESTART. "
                          << "Ignoring SKIPREST." << std::endl;
            }

            return;
        }

        const auto& rst_kw = deck.get<ParserKeywords::RESTART>().back();
        const auto& record = rst_kw.getRecord(0);

        if (const auto& save_item = record.getItem<ParserKeywords::RESTART::SAVEFILE>();
            save_item.hasValue(0))
        {
            throw OpmInputError {
                "OPM does not support RESTART from a SAVE file, "
                "only from RESTART files",
                rst_kw.location()
            };
        }

        const auto step = record
            .getItem<ParserKeywords::RESTART::REPORTNUMBER>()
            .get<int>(0);

        const auto input_path = std::filesystem::path {
            deck.getInputPath()
        };

        const auto input_root = std::filesystem::path {
            record.getItem<ParserKeywords::RESTART::ROOTNAME>().getTrimmedString(0)
        };

        const auto root = (input_root.is_absolute() || input_path.empty())
            ? input_root : input_path / input_root;

        this->m_restartRootNameInput = input_root.generic_string();

        this->setRestart(root.generic_string(), step);
    }

} // namespace Opm
