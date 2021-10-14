/*
  Copyright 2021 NORCE.

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
#ifndef OPM_PARSER_MICPPARA_HPP
#define	OPM_PARSER_MICPPARA_HPP

namespace Opm {

  class MICPpara {
  public:

      MICPpara();

      MICPpara( double density_biofilm , double density_calcite , double detachment_rate , double critical_porosity , double fitting_factor , double half_velocity_oxygen , double half_velocity_urea , double maximum_growth_rate , double maximum_oxygen_concentration , double maximum_urea_concentration , double maximum_urea_utilization , double microbial_attachment_rate , double microbial_death_rate , double minimum_permeability , double oxygen_consumption_factor , double tolerance_before_clogging , double yield_growth_coefficient) :
          m_density_biofilm( density_biofilm ),
          m_density_calcite( density_calcite ),
          m_detachment_rate( detachment_rate ),
          m_critical_porosity(  critical_porosity ),
          m_fitting_factor(  fitting_factor ),
          m_half_velocity_oxygen(  half_velocity_oxygen ),
          m_half_velocity_urea(  half_velocity_urea ),
          m_maximum_growth_rate(  maximum_growth_rate ),
          m_maximum_oxygen_concentration(  maximum_oxygen_concentration ),
          m_maximum_urea_concentration(  maximum_urea_concentration ),
          m_maximum_urea_utilization(  maximum_urea_utilization ),
          m_microbial_attachment_rate(  microbial_attachment_rate ),
          m_microbial_death_rate(  microbial_death_rate ),
          m_minimum_permeability(  minimum_permeability ),
          m_oxygen_consumption_factor(  oxygen_consumption_factor ),
          m_tolerance_before_clogging(  tolerance_before_clogging ),
          m_yield_growth_coefficient(  yield_growth_coefficient )
      { }

      static MICPpara serializeObject()
      {
          return MICPpara(1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16., 17.);
      }

      double getDensityBiofilm() const {
          return m_density_biofilm;
      }

      double getDensityCalcite() const {
          return m_density_calcite;
      }

      double getDetachmentRate() const {
          return m_detachment_rate;
      }

      double getCriticalPorosity() const {
          return m_critical_porosity;
      }

      double getFittingFactor() const {
          return m_fitting_factor;
      }

      double getHalfVelocityOxygen() const {
          return m_half_velocity_oxygen;
      }

      double getHalfVelocityUrea() const {
          return m_half_velocity_urea;
      }

      double getMaximumGrowthRate() const {
          return m_maximum_growth_rate;
      }

      double getMaximumOxygenConcentration() const {
          return m_maximum_oxygen_concentration;
      }

      double getMaximumUreaConcentration() const {
          return m_maximum_urea_concentration;
      }

      double getMaximumUreaUtilization() const {
          return m_maximum_urea_utilization;
      }

      double getMicrobialAttachmentRate() const {
          return m_microbial_attachment_rate;
      }

      double getMicrobialDeathRate() const {
          return m_microbial_death_rate;
      }

      double getMinimumPermeability() const {
          return m_minimum_permeability;
      }

      double getOxygenConsumptionFactor() const {
          return m_oxygen_consumption_factor;
      }

      double getToleranceBeforeClogging() const {
          return m_tolerance_before_clogging;
      }

      double getYieldGrowthCoefficient() const {
          return m_yield_growth_coefficient;
      }

      bool operator==(const MICPpara& data) const
      {
          return this->getDensityBiofilm() == data.getDensityBiofilm() &&
                 this->getDensityCalcite() == data.getDensityCalcite() &&
                 this->getDetachmentRate() == data.getDetachmentRate() &&
                 this->getCriticalPorosity() == data.getCriticalPorosity() &&
                 this->getFittingFactor() == data.getFittingFactor() &&
                 this->getHalfVelocityOxygen() == data.getHalfVelocityOxygen() &&
                 this->getHalfVelocityUrea() == data.getHalfVelocityUrea() &&
                 this->getMaximumGrowthRate() == data.getMaximumGrowthRate() &&
                 this->getMaximumOxygenConcentration() == data.getMaximumOxygenConcentration() &&
                 this->getMaximumUreaConcentration() == data.getMaximumUreaConcentration() &&
                 this->getMaximumUreaUtilization() == data.getMaximumUreaUtilization() &&
                 this->getMicrobialAttachmentRate() == data.getMicrobialAttachmentRate() &&
                 this->getMicrobialDeathRate() == data.getMicrobialDeathRate() &&
                 this->getMinimumPermeability() == data.getMinimumPermeability() &&
                 this->getOxygenConsumptionFactor() == data.getOxygenConsumptionFactor() &&
                 this->getToleranceBeforeClogging() == data.getToleranceBeforeClogging() &&
                 this->getYieldGrowthCoefficient() == data.getYieldGrowthCoefficient();
      }

      template<class Serializer>
      void serializeOp(Serializer& serializer)
      {
          serializer(m_density_biofilm);
          serializer(m_density_calcite);
          serializer(m_detachment_rate);
          serializer(m_critical_porosity);
          serializer(m_fitting_factor);
          serializer(m_half_velocity_oxygen);
          serializer(m_half_velocity_urea);
          serializer(m_maximum_growth_rate);
          serializer(m_maximum_oxygen_concentration);
          serializer(m_maximum_urea_concentration);
          serializer(m_maximum_urea_utilization);
          serializer(m_microbial_attachment_rate);
          serializer(m_microbial_death_rate);
          serializer(m_minimum_permeability);
          serializer(m_oxygen_consumption_factor);
          serializer(m_tolerance_before_clogging);
          serializer(m_yield_growth_coefficient);
      }

  private:
      double m_density_biofilm;
      double m_density_calcite;
      double m_detachment_rate;
      double m_critical_porosity;
      double m_fitting_factor;
      double m_half_velocity_oxygen;
      double m_half_velocity_urea;
      double m_maximum_growth_rate;
      double m_maximum_oxygen_concentration;
      double m_maximum_urea_concentration;
      double m_maximum_urea_utilization;
      double m_microbial_attachment_rate;
      double m_microbial_death_rate;
      double m_minimum_permeability;
      double m_oxygen_consumption_factor;
      double m_tolerance_before_clogging;
      double m_yield_growth_coefficient;

  };
}


#endif
