/*
  Copyright 2023 Equinor.

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
#ifndef OPM_PARSER_WAGHYSTERSISCONFIG_HPP
#define	OPM_PARSER_WAGHYSTERSISCONFIG_HPP

namespace Opm {

class Deck;
class DeckRecord;

  class WagHysteresisConfig {
  public:

      struct WagHysteresisConfigRecord {

      private:
          // WAG hysteresis Lands parameter
          double wagLandsParamValue  { 1.0 };
          // WAG hysteresis reduction factor
          double wagSecondaryDrainageReductionValue  { 0.0 };
          // WAG gas model flag
          bool wagGasFlagValue  { true };
          // WAG residual oil model flag
          bool wagResidualOilFlagValue  { false };
          // WAG water model flag
          bool wagWaterFlagValue  { false };
          // WAG hysteresis linear fraction
          double wagImbCurveLinearFractionValue  { 0.1 };
          // WAG hysteresis 3-phase threshold
          double wagWaterThresholdSaturationValue  { 0.001 };

      public:
          WagHysteresisConfigRecord() = default;

          explicit WagHysteresisConfigRecord(const DeckRecord& record);

          double wagLandsParam() const {
              return wagLandsParamValue;
          }

          double wagSecondaryDrainageReduction() const {
              return wagSecondaryDrainageReductionValue;
          }

          bool wagGasFlag() const {
              return wagGasFlagValue;
          }

          bool wagResidualOilFlag() const {
              return wagResidualOilFlagValue;
          }

          bool wagWaterFlag() const {
              return wagWaterFlagValue;
          }

          double wagImbCurveLinearFraction() const {
              return wagImbCurveLinearFractionValue;
          }

          double wagWaterThresholdSaturation() const {
              return wagWaterThresholdSaturationValue;
          }

          bool operator==(const WagHysteresisConfigRecord& data) const
          {
              return this->wagLandsParam() == data.wagLandsParam() &&
               this->wagSecondaryDrainageReduction() == data.wagSecondaryDrainageReduction() &&
               this->wagGasFlag() == data.wagGasFlag() &&
               this->wagResidualOilFlag() == data.wagResidualOilFlag() &&
               this->wagWaterFlag() == data.wagWaterFlag() &&
               this->wagImbCurveLinearFraction() == data.wagImbCurveLinearFraction() &&
               this->wagWaterThresholdSaturation() == data.wagWaterThresholdSaturation();
          }

          template<class Serializer>
          void serializeOp(Serializer& serializer)
          {
              serializer(wagLandsParamValue);
              serializer(wagSecondaryDrainageReductionValue);
              serializer(wagGasFlagValue);
              serializer(wagResidualOilFlagValue);
              serializer(wagWaterFlagValue);
              serializer(wagImbCurveLinearFractionValue);
              serializer(wagWaterThresholdSaturationValue);
          }

          static WagHysteresisConfigRecord serializationTestObject()
          {
              WagHysteresisConfigRecord result;
              result.wagLandsParamValue = 0;
              result.wagSecondaryDrainageReductionValue = 1;
              result.wagGasFlagValue = true;
              result.wagResidualOilFlagValue = false;
              result.wagWaterFlagValue = false;
              result.wagImbCurveLinearFractionValue = 2;
              result.wagWaterThresholdSaturationValue = 3;

              return result;
          }
      };

      WagHysteresisConfig();

      explicit WagHysteresisConfig(const Deck& deck);

      size_t size() const;
      bool empty() const;

      const std::vector<WagHysteresisConfigRecord>::const_iterator begin() const;
      const std::vector<WagHysteresisConfigRecord>::const_iterator end() const;

      template<class Serializer>
      void serializeOp(Serializer& serializer)
      {
        serializer(wagrecords);
      }
      bool operator==(const WagHysteresisConfig& other) const;

      const WagHysteresisConfigRecord& operator[](std::size_t index) const;

  private:
      std::vector<WagHysteresisConfigRecord> wagrecords;
  };
}

#endif // OPM_PARSER_WAGHYSTERSISCONFIG_HPP
