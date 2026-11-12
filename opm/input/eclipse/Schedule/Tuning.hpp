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

#ifndef OPM_TUNING_HPP
#define OPM_TUNING_HPP

#include <optional>

namespace Opm {

    class NextStep {
    public:
        NextStep() = default;
        NextStep(double value, bool every_report);
        double value() const;
        bool every_report() const;
        bool operator==(const NextStep& other) const;
        static NextStep serializationTestObject();

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->next_tstep);
            serializer(this->persist);
        }

    private:
        double next_tstep{};
        bool persist{false};
    };

    struct Tuning {
        Tuning();

        static Tuning serializationTestObject();

        // Record1
        std::optional<double> TSINIT;
        double TSMAXZ;
        double TSMINZ;
        double TSMCHP;
        double TSFMAX;
        double TSFMIN;
        double TFDIFF;
        double TSFCNV;
        double THRUPT;
        double TMAXWC = 0.0;
        bool TMAXWC_has_value = false;

        // Record 2
        double TRGTTE;
        bool TRGTTE_has_value = false;
        double TRGCNV;
        double TRGMBE;
        double TRGLCV;
        bool TRGLCV_has_value = false;
        double XXXTTE;
        bool XXXTTE_has_value = false;
        double XXXCNV;
        double XXXMBE;
        double XXXLCV;
        bool XXXLCV_has_value = false;
        double XXXWFL;
        bool XXXWFL_has_value = false;
        double TRGFIP;
        bool TRGFIP_has_value = false;
        double TRGSFT = 0.0;
        bool TRGSFT_has_value = false;
        double THIONX;
        bool THIONX_has_value = false;
        double TRWGHT;
        bool TRWGHT_has_value = false;

        // Record 3
        int NEWTMX;
        int NEWTMN;
        int LITMAX;
        bool LITMAX_has_value = false;
        int LITMIN;
        bool LITMIN_has_value = false;
        int MXWSIT;
        bool MXWSIT_has_value = false;
        int MXWPIT;
        bool MXWPIT_has_value = false;
        double DDPLIM;
        bool DDPLIM_has_value = false;
        double DDSLIM;
        bool DDSLIM_has_value = false;
        double TRGDPR;
        bool TRGDPR_has_value = false;
        double XXXDPR;
        bool XXXDPR_has_value = false;
        int MNWRFP;
        bool MNWRFP_has_value = false;

        /*
          In addition to the values set in the TUNING keyword this Tuning
          implementation also contains the result of the WSEGITER keyword, which
          is special tuning parameters to be applied to the multisegment well
          model. Observe that the maximum number of well iterations - MXWSIT -
          is specified by both the TUNING keyword and the WSEGITER keyword, but
          with different defaults.
        */
        int WSEG_MAX_RESTART;
        double WSEG_REDUCTION_FACTOR;
        double WSEG_INCREASE_FACTOR;


        bool operator==(const Tuning& data) const;
        bool operator !=(const Tuning& data) const {
            return !(*this == data);
        }

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(TSINIT);
            serializer(TSMAXZ);
            serializer(TSMINZ);
            serializer(TSMCHP);
            serializer(TSFMAX);
            serializer(TSFMIN);
            serializer(TFDIFF);
            serializer(TSFCNV);
            serializer(THRUPT);
            serializer(TMAXWC);
            serializer(TMAXWC_has_value);

            serializer(TRGTTE);
            serializer(TRGTTE_has_value);
            serializer(TRGCNV);
            serializer(TRGMBE);
            serializer(TRGLCV);
            serializer(TRGLCV_has_value);
            serializer(XXXTTE);
            serializer(XXXTTE_has_value);
            serializer(XXXCNV);
            serializer(XXXMBE);
            serializer(XXXLCV);
            serializer(XXXLCV_has_value);
            serializer(XXXWFL);
            serializer(XXXWFL_has_value);
            serializer(TRGFIP);
            serializer(TRGFIP_has_value);
            serializer(TRGSFT);
            serializer(TRGSFT_has_value);
            serializer(THIONX);
            serializer(THIONX_has_value);
            serializer(TRWGHT);
            serializer(TRWGHT_has_value);

            serializer(NEWTMX);
            serializer(NEWTMN);
            serializer(LITMAX);
            serializer(LITMAX_has_value);
            serializer(LITMIN);
            serializer(LITMIN_has_value);
            serializer(MXWSIT);
            serializer(MXWSIT_has_value);
            serializer(MXWPIT);
            serializer(MXWPIT_has_value);
            serializer(DDPLIM);
            serializer(DDPLIM_has_value);
            serializer(DDSLIM);
            serializer(DDSLIM_has_value);
            serializer(TRGDPR);
            serializer(TRGDPR_has_value);
            serializer(XXXDPR);
            serializer(XXXDPR_has_value);
            serializer(MNWRFP);
            serializer(MNWRFP_has_value);

            serializer(WSEG_MAX_RESTART);
            serializer(WSEG_REDUCTION_FACTOR);
            serializer(WSEG_INCREASE_FACTOR);
        }
    };

    struct TuningDp {
        TuningDp();

        static TuningDp serializationTestObject();

        // NOTE: TRGLCV and XXXLCV are the same as in TUNING, since they define a different default value in TUNINGDP
        double TRGLCV;
        bool TRGLCV_has_value{false};
        double XXXLCV;
        bool XXXLCV_has_value{false};
        double TRGDDP;
        double TRGDDS;
        double TRGDDRS;
        double TRGDDRV;

        bool defaults_updated{false};

        void set_defaults();

        bool operator==(const TuningDp& other) const;
        bool operator!=(const TuningDp& other) const
        {
            return !(*this == other);
        }

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(TRGLCV);
            serializer(TRGLCV_has_value);
            serializer(XXXLCV);
            serializer(XXXLCV_has_value);
            serializer(TRGDDP);
            serializer(TRGDDS);
            serializer(TRGDDRS);
            serializer(TRGDDRV);
            serializer(defaults_updated);
        }
    };

    /// Convergence criteria and iteration limits from the CVCRIT keyword,
    /// in SI units.
    ///
    /// Every item has a default, so an item defaulted in the deck takes
    /// that default rather than its value from an earlier CVCRIT.  The
    /// items whose default depends on the solution method have no value
    /// until the deck sets them.  Items for SAMG, Appleyard chopping and
    /// the extra linear residual checks are not stored.
    struct Cvcrit {
        Cvcrit();

        static Cvcrit serializationTestObject();

        double pressure_change_tolerance;                        // Item 1
        std::optional<int> max_nonlinear_iterations;             // Item 2
        double linear_residual_reduction;                        // Item 3
        std::optional<int> max_linear_iterations;                // Item 4
        double fugacity_error_tolerance;                         // Item 5
        std::optional<int> min_linear_iterations;                // Item 6
        std::optional<double> specific_volume_change_tolerance;  // Item 7
        double temperature_change_tolerance;                     // Item 8
        std::optional<int> min_nonlinear_iterations;             // Item 9
        double residual_max_norm_tolerance;                      // Item 11
        double residual_sum_tolerance;                           // Item 12
        std::optional<double> max_molar_density_change;          // Item 13, no unit conversion
        std::optional<double> max_temperature_change;            // Item 14
        std::optional<double> max_pressure_change;               // Item 15
        int msw_max_iterations;                                  // Item 23
        double msw_molar_density_tolerance;                      // Item 24
        double msw_molar_rate_tolerance;                         // Item 25
        double msw_energy_density_tolerance;                     // Item 26
        double msw_velocity_tolerance;                           // Item 27
        double msw_molar_residual_tolerance;                     // Item 31
        double msw_energy_residual_tolerance;                    // Item 32
        double msw_max_pressure_change;                          // Item 37
        double msw_max_specific_volume_change;                   // Item 38
        double msw_max_temperature_change;                       // Item 39
        double additional_residual_max_norm_tolerance;           // Item 41
        double additional_residual_sum_tolerance;                // Item 42

        bool operator==(const Cvcrit&) const = default;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(pressure_change_tolerance);
            serializer(max_nonlinear_iterations);
            serializer(linear_residual_reduction);
            serializer(max_linear_iterations);
            serializer(fugacity_error_tolerance);
            serializer(min_linear_iterations);
            serializer(specific_volume_change_tolerance);
            serializer(temperature_change_tolerance);
            serializer(min_nonlinear_iterations);
            serializer(residual_max_norm_tolerance);
            serializer(residual_sum_tolerance);
            serializer(max_molar_density_change);
            serializer(max_temperature_change);
            serializer(max_pressure_change);
            serializer(msw_max_iterations);
            serializer(msw_molar_density_tolerance);
            serializer(msw_molar_rate_tolerance);
            serializer(msw_energy_density_tolerance);
            serializer(msw_velocity_tolerance);
            serializer(msw_molar_residual_tolerance);
            serializer(msw_energy_residual_tolerance);
            serializer(msw_max_pressure_change);
            serializer(msw_max_specific_volume_change);
            serializer(msw_max_temperature_change);
            serializer(additional_residual_max_norm_tolerance);
            serializer(additional_residual_sum_tolerance);
        }
    };

} //namespace Opm

#endif
