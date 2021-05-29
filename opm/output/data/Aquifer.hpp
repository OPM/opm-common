/*
  Copyright 2019 Statoil ASA.

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

#ifndef OPM_OUTPUT_AQUIFER_HPP
#define OPM_OUTPUT_AQUIFER_HPP

#include <map>
#include <memory>
#include <string>

namespace Opm { namespace data {

    /**
     * Small struct that keeps track of data for output to restart/summary
     * files.
     */
    enum class AquiferType
    {
        Fetkovich, CarterTracy, Numerical,
    };

    struct FetkovichData
    {
        double initVolume{};
        double prodIndex{};
        double timeConstant{};

        bool operator==(const FetkovichData& other) const;

        // MessageBufferType API should be similar to Dune::MessageBufferIF
        template <class MessageBufferType>
        void write(MessageBufferType& buffer) const;

        // MessageBufferType API should be similar to Dune::MessageBufferIF
        template <class MessageBufferType>
        void read(MessageBufferType& buffer);
    };

    struct CarterTracyData
    {
        double timeConstant{};
        double influxConstant{};
        double waterDensity{};
        double waterViscosity{};

        double dimensionless_time{};
        double dimensionless_pressure{};

        bool operator==(const CarterTracyData& other) const;

        // MessageBufferType API should be similar to Dune::MessageBufferIF
        template <class MessageBufferType>
        void write(MessageBufferType& buffer) const;

        // MessageBufferType API should be similar to Dune::MessageBufferIF
        template <class MessageBufferType>
        void read(MessageBufferType& buffer);
    };

    struct AquiferData
    {
        int aquiferID = 0;         //< One-based ID, range 1..NANAQ
        double pressure = 0.0;     //< Aquifer pressure
        double fluxRate = 0.0;     //< Aquifer influx rate (liquid aquifer)
        double volume = 0.0;       //< Produced liquid volume
        double initPressure = 0.0; //< Aquifer's initial pressure
        double datumDepth = 0.0;   //< Aquifer's pressure reference depth

        AquiferType type;
        std::shared_ptr<FetkovichData> aquFet{};
        std::shared_ptr<CarterTracyData> aquCT{};

        double get(const std::string& key) const;

        bool operator==(const AquiferData& other) const;

        // MessageBufferType API should be similar to Dune::MessageBufferIF
        template <class MessageBufferType>
        void write(MessageBufferType& buffer) const;

        // MessageBufferType API should be similar to Dune::MessageBufferIF
        template <class MessageBufferType>
        void read(MessageBufferType& buffer);
    };

    // TODO: not sure what extension we will need
    using Aquifers = std::map<int, AquiferData>;

    inline bool FetkovichData::operator==(const FetkovichData& other) const
    {
        return (this->initVolume == other.initVolume)
            && (this->prodIndex == other.prodIndex)
            && (this->timeConstant == other.timeConstant);
    }

    template <class MessageBufferType>
    void FetkovichData::write(MessageBufferType& buffer) const
    {
        buffer.write(this->initVolume);
        buffer.write(this->prodIndex);
        buffer.write(this->timeConstant);
    }

    template <class MessageBufferType>
    void FetkovichData::read(MessageBufferType& buffer)
    {
        buffer.read(this->initVolume);
        buffer.read(this->prodIndex);
        buffer.read(this->timeConstant);
    }

    inline bool CarterTracyData::operator==(const CarterTracyData& other) const
    {
        return (this->timeConstant == other.timeConstant)
            && (this->influxConstant == other.influxConstant)
            && (this->waterDensity == other.waterDensity)
            && (this->waterViscosity == other.waterViscosity)
            && (this->dimensionless_time == other.dimensionless_time)
            && (this->dimensionless_pressure == other.dimensionless_pressure);
    }

    template <class MessageBufferType>
    void CarterTracyData::write(MessageBufferType& buffer) const
    {
        buffer.write(this->timeConstant);
        buffer.write(this->influxConstant);
        buffer.write(this->waterDensity);
        buffer.write(this->waterViscosity);
        buffer.write(this->dimensionless_time);
        buffer.write(this->dimensionless_pressure);
    }

    template <class MessageBufferType>
    void CarterTracyData::read(MessageBufferType& buffer)
    {
        buffer.read(this->timeConstant);
        buffer.read(this->influxConstant);
        buffer.read(this->waterDensity);
        buffer.read(this->waterViscosity);
        buffer.read(this->dimensionless_time);
        buffer.read(this->dimensionless_pressure);
    }

    inline double AquiferData::get(const std::string& key) const
    {
        if ((key == "AAQR") || (key == "ANQR")) {
            return this->fluxRate;
        }
        else if ((key == "AAQT") || (key == "ANQT")) {
            return this->volume;
        }
        else if ((key == "AAQP") || (key == "ANQP")) {
            return this->pressure;
        }
        else if ((key == "AAQTD") && (this->aquCT != nullptr)) {
            return this->aquCT->dimensionless_time;
        }
        else if ((key == "AAQPD") && (this->aquCT != nullptr)) {
            return this->aquCT->dimensionless_pressure;
        }

        return 0.0;
    }

    inline bool AquiferData::operator==(const AquiferData& other) const
    {
        const auto equal_structure =
            (this->aquiferID == other.aquiferID) &&
            (this->pressure == other.pressure) &&
            (this->fluxRate == other.fluxRate) &&
            (this->volume == other.volume) &&
            (this->initPressure == other.initPressure) &&
            (this->datumDepth == other.datumDepth) &&
            (this->type == other.type) &&
            ((this->aquFet == nullptr) == (other.aquFet == nullptr)) &&
            ((this->aquCT == nullptr) == (other.aquCT == nullptr)) &&
            ((this->aquFet == nullptr) != (this->aquCT == nullptr));

        if (! equal_structure) {
            return false;
        }

        auto equalSub = true;
        if (this->aquFet != nullptr) {
            equalSub = *this->aquFet == *other.aquFet;
        }

        if (equalSub && (this->aquCT != nullptr)) {
            equalSub = *this->aquCT == *other.aquCT;
        }

        return equalSub;
    }

    template <class MessageBufferType>
    void AquiferData::write(MessageBufferType& buffer) const
    {
        buffer.write(this->aquiferID);
        buffer.write(this->pressure);
        buffer.write(this->fluxRate);
        buffer.write(this->volume);
        buffer.write(this->initPressure);
        buffer.write(this->datumDepth);

        const int aqu = (this->aquFet != nullptr) + 2*(this->aquCT != nullptr);
        buffer.write(aqu);

        if (this->aquFet != nullptr) {
            this->aquFet->write(buffer);
        }
        else if (this->aquCT != nullptr) {
            this->aquCT->write(buffer);
        }
    }

    template <class MessageBufferType>
    void AquiferData::read(MessageBufferType& buffer)
    {
        buffer.read(this->aquiferID);
        buffer.read(this->pressure);
        buffer.read(this->fluxRate);
        buffer.read(this->volume);
        buffer.read(this->initPressure);
        buffer.read(this->datumDepth);

        int aqu;
        buffer.read(aqu);
        if (aqu == 1) {
            this->type = AquiferType::Fetkovich;

            if (this->aquFet == nullptr) {
                this->aquFet = std::make_shared<FetkovichData>();
            }

            this->aquFet->read(buffer);
        }
        else if (aqu == 2) {
            this->type = AquiferType::CarterTracy;

            if (this->aquCT == nullptr) {
                this->aquCT = std::make_shared<CarterTracyData>();
            }

            this->aquCT->read(buffer);
        }
    }
}} // Opm::data

#endif // OPM_OUTPUT_AQUIFER_HPP
