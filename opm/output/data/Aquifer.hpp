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
        double initVolume;
        double prodIndex;
        double timeConstant;
    };

    struct CarterTracyData
    {
        double dimensionless_time;
        double dimensionless_pressure;
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
        std::shared_ptr<FetkovichData> aquFet;
        std::shared_ptr<CarterTracyData> aquCT;

        double get(const std::string& key) const;

        // MessageBufferType API should be similar to Dune::MessageBufferIF
        template <class MessageBufferType>
        void write(MessageBufferType& buffer) const;

        // MessageBufferType API should be similar to Dune::MessageBufferIF
        template <class MessageBufferType>
        void read(MessageBufferType& buffer);
    };

    // TODO: not sure what extension we will need
    using Aquifers = std::map<int, AquiferData>;

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
            buffer.write(this->aquFet->initVolume);
            buffer.write(this->aquFet->prodIndex);
            buffer.write(this->aquFet->timeConstant);
        }
        else if (this->aquCT != nullptr) {
            buffer.write(this->aquCT->dimensionless_time);
            buffer.write(this->aquCT->dimensionless_pressure);
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

            buffer.read(this->aquFet->initVolume);
            buffer.read(this->aquFet->prodIndex);
            buffer.read(this->aquFet->timeConstant);
        }
        else if (aqu == 2) {
            this->type = AquiferType::CarterTracy;

            if (this->aquCT == nullptr) {
                this->aquCT = std::make_shared<CarterTracyData>();
            }

            buffer.read(this->aquCT->dimensionless_time);
            buffer.read(this->aquCT->dimensionless_pressure);
        }
    }
}} // Opm::data

#endif // OPM_OUTPUT_AQUIFER_HPP
