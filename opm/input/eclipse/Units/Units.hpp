//===========================================================================
//
// File: Units.hpp
//
// Created: Thu Jul  2 09:19:08 2009
//
// Author(s): Halvor M Nilsen <hnil@sintef.no>
//
// $Date$
//
// $Revision$
//
//===========================================================================

/*
  Copyright 2009, 2010, 2011, 2012 SINTEF ICT, Applied Mathematics.
  Copyright 2009, 2010, 2011, 2012 Statoil ASA.

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

#ifndef OPM_UNITS_HEADER
#define OPM_UNITS_HEADER

/**
   The unit sets emplyed in ECLIPSE, in particular the FIELD units,
   are quite inconsistent. Ideally one should choose units for a set
   of base quantities like Mass,Time and Length and then derive the
   units for e.g. pressure and flowrate in a consisten manner. However
   that is not the case; for instance in the metric system we have:

      [Length] = meters
      [time] = days
      [mass] = kg

   This should give:

      [Pressure] = [mass] / ([length] * [time]^2) = kg / (m * days * days)

   Instead pressure is given in Bars. When it comes to FIELD units the
   number of such examples is long.
*/
namespace Opm {
    namespace prefix
    /// Conversion prefix for units.
    {
        constexpr double micro = 1.0e-6;  /**< Unit prefix [\f$\mu\f$] */
        constexpr double milli = 1.0e-3;  /**< Unit prefix [m] */
        constexpr double centi = 1.0e-2;  /**< Non-standard unit prefix [c] */
        constexpr double deci  = 1.0e-1;  /**< Non-standard unit prefix [d] */
        constexpr double kilo  = 1.0e3;   /**< Unit prefix [k] */
        constexpr double mega  = 1.0e6;   /**< Unit prefix [M] */
        constexpr double giga  = 1.0e9;   /**< Unit prefix [G] */
    } // namespace prefix

    namespace unit
    /// Definition of various units.
    /// All the units are defined in terms of international standard
    /// units (SI).  Example of use: We define a variable \c k which
    /// gives a permeability. We want to set \c k to \f$1\,mD\f$.
    /// \code
    /// using namespace Opm::unit
    /// double k = 0.001*darcy;
    /// \endcode
    /// We can also use one of the prefixes defined in Opm::prefix
    /// \code
    /// using namespace Opm::unit
    /// using namespace Opm::prefix
    /// double k = 1.0*milli*darcy;
    /// \endcode
    {
        ///\name Common powers
        /// @{
        constexpr double square(double v) { return v * v;     }
        constexpr double cubic (double v) { return v * v * v; }
        /// @}

        // --------------------------------------------------------------
        // Basic (fundamental) units and conversions
        // --------------------------------------------------------------

        /// \name Length
        /// @{
        constexpr double meter =  1;
        constexpr double inch  =  2.54 * prefix::centi*meter;
        constexpr double feet  = 12    * inch;
        /// @}

        /// \name Time
        /// @{
        constexpr double second   =   1;
        constexpr double minute   =  60    * second;
        constexpr double hour     =  60    * minute;
        constexpr double day      =  24    * hour;
        constexpr double year     = 365    * day;
        constexpr double ecl_year = 365.25 * day;
        /// @}

        /// \name Volume
        /// @{
        constexpr double gallon = 231 * cubic(inch);
        constexpr double stb    =  42 * gallon;
        constexpr double liter  =   1 * cubic(prefix::deci*meter);
        /// @}

        /// \name Mass
        /// @{
        constexpr double kilogram = 1;
        constexpr double gram     = 1.0e-3 * kilogram;
        // http://en.wikipedia.org/wiki/Pound_(mass)#Avoirdupois_pound
        constexpr double pound    = 0.45359237 * kilogram;
        /// @}

        /// \name Energy
        /// @{
        constexpr double joule = 1;
        constexpr double btu = 1054.3503*joule; // "british thermal units"
        /// @}

        // --------------------------------------------------------------
        // Standardised constants
        // --------------------------------------------------------------

        /// \name Standardised constant
        /// @{
        constexpr double gravity = 9.80665 * meter/square(second);
        /// @}

        constexpr double mol = 1;
        // --------------------------------------------------------------
        // Derived units and conversions
        // --------------------------------------------------------------

        /// \name Force
        /// @{
        constexpr double Newton = kilogram*meter / square(second); // == 1
        constexpr double dyne   = 1e-5*Newton;
        constexpr double lbf    = pound * gravity; // Pound-force
        /// @}

        /// \name Pressure
        /// @{
        constexpr double Pascal = Newton / square(meter); // == 1
        constexpr double barsa  = 100000 * Pascal;
        constexpr double bars   = 100000 * Pascal;
        constexpr double atma   = 101325 * Pascal;
        constexpr double atm    = 101325 * Pascal;
        constexpr double psia   = lbf / square(inch);
        constexpr double psi    = lbf / square(inch);
        /// @}

        /// \name Temperature. This one is more complicated
        /// because the unit systems used by Eclipse (i.e. degrees
        /// Celsius and degrees Fahrenheit require to add or
        /// subtract an offset for the conversion between from/to
        /// Kelvin
        /// @{
        constexpr double degCelsius = 1.0; // scaling factor °C -> K
        constexpr double degCelsiusOffset = 273.15; // offset for the °C -> K conversion

        constexpr double degFahrenheit = 5.0/9.0; // factor to convert a difference in °F to a difference in K
        constexpr double degFahrenheitOffset = 459.67*degFahrenheit; // °F -> K offset (i.e. how many K is 0 °F?)
        /// @}

        /// \name Viscosity
        /// @{
        constexpr double Pas   = Pascal * second; // == 1
        constexpr double Poise = prefix::deci*Pas;
        /// @}

        constexpr double ppm = 1. / prefix::mega;

        namespace perm_details {
            constexpr double p_grad   = atm / (prefix::centi*meter);
            constexpr double area     = square(prefix::centi*meter);
            constexpr double flux     = cubic (prefix::centi*meter) / second;
            constexpr double velocity = flux / area;
            constexpr double visc     = prefix::centi*Poise;
            constexpr double darcy    = (velocity * visc) / p_grad;
            //                    == 1e-7 [m^2] / 101325
            //                    == 9.869232667160130e-13 [m^2]
        }
        /// \name Permeability
        /// @{
        ///
        /// A porous medium with a permeability of 1 darcy permits a flow (flux)
        /// of \f$1\,\mathit{cm}^3/s\f$ of a fluid with viscosity
        /// \f$1\,\mathit{cP}\f$ (\f$1\,mPa\cdot s\f$) under a pressure gradient
        /// of \f$1\,\mathit{atm}/\mathit{cm}\f$ acting across an area of
        /// \f$1\,\mathit{cm}^2\f$.
        ///
        constexpr double darcy = perm_details::darcy;
        /// @}

        /**
         * Unit conversion routines.
         */
        namespace convert {
            /**
             * Convert from external units of measurements to equivalent
             * internal units of measurements.  Note: The internal units of
             * measurements are *ALWAYS*, and exclusively, SI.
             *
             * Example: Convert a double @c kx, containing a permeability value
             * in units of milli-darcy (mD) to the equivalent value in SI units
             * (i.e., \f$m^2\f$).
             * \code
             *    using namespace Opm::unit;
             *    using namespace Opm::prefix;
             *    convert::from(kx, milli*darcy);
             * \endcode
             *
             * @param[in] q    Physical quantity.
             * @param[in] unit Physical unit of measurement.
             * @return Value of @c q in equivalent SI units of measurements.
             */
            constexpr double from(const double q, const double unit)
            {
                return q * unit;
            }

            /**
             * Convert from internal units of measurements to equivalent
             * external units of measurements.  Note: The internal units of
             * measurements are *ALWAYS*, and exclusively, SI.
             *
             * Example: Convert a <CODE>std::vector<double> p</CODE>, containing
             * pressure values in the SI unit Pascal (i.e., unit::Pascal) to the
             * equivalent values in Psi (unit::psia).
             * \code
             *    using namespace Opm::unit;
             *    std::ranges::transform(p, p.begin(),
              *                         std::bind(convert::to, std::placeholders::_1, psia));
             * \endcode
             *
             * @param[in] q    Physical quantity, measured in SI units.
             * @param[in] unit Physical unit of measurement.
             * @return Value of @c q in unit <CODE>unit</CODE>.
             */
            constexpr double to(const double q, const double unit)
            {
                return q / unit;
            }
        } // namespace convert
    }

    namespace Metric {
        using namespace prefix;
        using namespace unit;
        constexpr double Pressure             = barsa;
        constexpr double PressureDrop         = bars;
        constexpr double Temperature          = degCelsius;
        constexpr double TemperatureOffset    = degCelsiusOffset;
        constexpr double AbsoluteTemperature  = degCelsius; // actually [K], but the these two are identical
        constexpr double Length               = meter;
        constexpr double Time                 = day;
        constexpr double RunTime              = second;
        constexpr double Mass                 = kilogram;
        constexpr double Permeability         = milli*darcy;
        constexpr double Transmissibility     = centi*Poise*cubic(meter)/(day*barsa);
        constexpr double LiquidSurfaceVolume  = cubic(meter);
        constexpr double GasSurfaceVolume     = cubic(meter);
        constexpr double ReservoirVolume      = cubic(meter);
        constexpr double Area                 = square(meter);
        constexpr double GeomVolume           = cubic(meter);
        constexpr double GasDissolutionFactor = GasSurfaceVolume/LiquidSurfaceVolume;
        constexpr double OilDissolutionFactor = LiquidSurfaceVolume/GasSurfaceVolume;
        constexpr double Density              = kilogram/cubic(meter);
        constexpr double Concentration        = kilogram/cubic(meter);
        constexpr double FoamDensity          = kilogram/cubic(meter);
        constexpr double Viscosity            = centi*Poise;
        constexpr double Timestep             = day;
        constexpr double SurfaceTension       = dyne/(centi*meter);
        constexpr double Energy               = kilo*joule;
        constexpr double Moles                = kilo*mol;
        constexpr double PPM                  = ppm;
        constexpr double Ymodule              = giga*Pascal;
        constexpr double ThermalConductivity  = kilo*joule/(meter*day*degCelsius);
    }


    namespace Field {
        using namespace prefix;
        using namespace unit;
        constexpr double Pressure             = psia;
        constexpr double PressureDrop         = psi;
        constexpr double Temperature          = degFahrenheit;
        constexpr double TemperatureOffset    = degFahrenheitOffset;
        constexpr double AbsoluteTemperature  = degFahrenheit; // actually [°R], but the these two are identical
        constexpr double Length               = feet;
        constexpr double Time                 = day;
        constexpr double RunTime              = second;
        constexpr double Mass                 = pound;
        constexpr double Permeability         = milli*darcy;
        constexpr double Transmissibility     = centi*Poise*stb/(day*psia);
        constexpr double LiquidSurfaceVolume  = stb;
        constexpr double GasSurfaceVolume     = 1000*cubic(feet);
        constexpr double ReservoirVolume      = stb;
        constexpr double Area                 = square(feet);
        constexpr double GeomVolume           = cubic(feet);
        constexpr double GasDissolutionFactor = GasSurfaceVolume/LiquidSurfaceVolume;
        constexpr double OilDissolutionFactor = LiquidSurfaceVolume/GasSurfaceVolume;
        constexpr double Density              = pound/cubic(feet);
        constexpr double Concentration        = pound/stb;
        constexpr double FoamDensity          = pound/GasSurfaceVolume;
        constexpr double Viscosity            = centi*Poise;
        constexpr double Timestep             = day;
        constexpr double SurfaceTension       = dyne/(centi*meter);
        constexpr double Energy               = btu;
        constexpr double Moles                = kilo*pound*mol;
        constexpr double PPM                  = ppm;
        constexpr double Ymodule              = giga*Pascal;
        constexpr double ThermalConductivity  = btu/(feet*day*degFahrenheit);
    }


    namespace Lab {
        using namespace prefix;
        using namespace unit;
        constexpr double Pressure             = atma;
        constexpr double PressureDrop         = atm;
        constexpr double Temperature          = degCelsius;
        constexpr double TemperatureOffset    = degCelsiusOffset;
        constexpr double AbsoluteTemperature  = degCelsius; // actually [K], but the these two are identical
        constexpr double Length               = centi*meter;
        constexpr double Time                 = hour;
        constexpr double RunTime              = second;
        constexpr double Mass                 = gram;
        constexpr double Permeability         = milli*darcy;
        constexpr double Transmissibility     = centi*Poise*cubic(centi*meter)/(hour*atm);
        constexpr double LiquidSurfaceVolume  = cubic(centi*meter);
        constexpr double GasSurfaceVolume     = cubic(centi*meter);
        constexpr double ReservoirVolume      = cubic(centi*meter);
        constexpr double Area                 = square(centi*meter);
        constexpr double GeomVolume           = cubic(centi*meter);
        constexpr double GasDissolutionFactor = GasSurfaceVolume/LiquidSurfaceVolume;
        constexpr double OilDissolutionFactor = LiquidSurfaceVolume/GasSurfaceVolume;
        constexpr double Density              = gram/cubic(centi*meter);
        constexpr double Concentration        = gram/cubic(centi*meter);
        constexpr double FoamDensity          = gram/cubic(centi*meter);
        constexpr double Viscosity            = centi*Poise;
        constexpr double Timestep             = hour;
        constexpr double SurfaceTension       = dyne/(centi*meter);
        constexpr double Energy               = joule;
        constexpr double Moles                = mol;
        constexpr double PPM                  = ppm;
        constexpr double Ymodule              = giga*Pascal;
        constexpr double ThermalConductivity  = joule/(centi*meter*hour*degCelsius);
    }


    namespace PVT_M {
        using namespace prefix;
        using namespace unit;
        constexpr double Pressure             = atma;
        constexpr double PressureDrop         = atm;
        constexpr double Temperature          = degCelsius;
        constexpr double TemperatureOffset    = degCelsiusOffset;
        constexpr double AbsoluteTemperature  = degCelsius; // actually [K], but the these two are identical
        constexpr double Length               = meter;
        constexpr double Time                 = day;
        constexpr double RunTime              = second;
        constexpr double Mass                 = kilogram;
        constexpr double Permeability         = milli*darcy;
        constexpr double Transmissibility     = centi*Poise*cubic(meter)/(day*atm);
        constexpr double LiquidSurfaceVolume  = cubic(meter);
        constexpr double GasSurfaceVolume     = cubic(meter);
        constexpr double ReservoirVolume      = cubic(meter);
        constexpr double Area                 = square(meter);
        constexpr double GeomVolume           = cubic(meter);
        constexpr double GasDissolutionFactor = GasSurfaceVolume/LiquidSurfaceVolume;
        constexpr double OilDissolutionFactor = LiquidSurfaceVolume/GasSurfaceVolume;
        constexpr double Density              = kilogram/cubic(meter);
        constexpr double Concentration        = kilogram/cubic(meter);
        constexpr double FoamDensity          = kilogram/cubic(meter);
        constexpr double Viscosity            = centi*Poise;
        constexpr double Timestep             = day;
        constexpr double SurfaceTension       = dyne/(centi*meter);
        constexpr double Energy               = kilo*joule;
        constexpr double Moles                = kilo*mol;
        constexpr double PPM                  = ppm;
        constexpr double Ymodule              = giga*Pascal;
        constexpr double ThermalConductivity  = kilo*joule/(meter*day*degCelsius);
    }
}

#endif // OPM_UNITS_HEADER
