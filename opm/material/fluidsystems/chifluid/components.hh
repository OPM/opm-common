#ifndef COMPONENTS_HH
#define COMPONENTS_HH

#include "chiwoms.h"

#include <opm/material/IdealGas.hpp>
#include <opm/material/components/Component.hpp>
#include <opm/material/components/SimpleCO2.hpp>
#include <opm/material/components/CO2.hpp>
#include <opm/material/components/H2O.hpp>

namespace Opm {
/*!
 * \ingroup Components
 *
 * \brief A simple representation of linear octane
 *
 * \tparam Scalar The type used for scalar values
 */
template <class Scalar>
class Octane : public Opm::Component<Scalar, Octane<Scalar> >
{
public:
        /// Chemical name
        static const char* name() { return "C8"; }

        /// Molar mass in \f$\mathrm{[kg/mol]}\f$
        static Scalar molarMass() { return 0.11423; }

        /// Critical temperature in \f$\mathrm[K]}\f$
        static Scalar criticalTemperature() { return 568.7; }

        /// Critical pressure in \f$\mathrm[Pa]}\f$
        static Scalar criticalPressure() { return 2.49e6; }

        /// Acentric factor
        static Scalar acentricFactor() { return 0.398; }

        // Critical volume [m3/kmol] (same as [L/mol])
        static Scalar criticalVolume() {return 4.92e-1; }
};

template <class Scalar>
class NDekane : public Opm::Component<Scalar, NDekane<Scalar> >
{
public:
        /// Chemical name
        static const char* name() { return "C10"; }

        /// Molar mass in \f$\mathrm{[kg/mol]}\f$
        static Scalar molarMass() { return 0.1423; }

        /// Critical temperature in \f$\mathrm[K]}\f$
        static Scalar criticalTemperature() { return 617.7; }

        /// Critical pressure in \f$\mathrm[Pa]}\f$
        static Scalar criticalPressure() { return 2.103e6; }

        /// Acentric factor
        static Scalar acentricFactor() { return 0.4884; }

        // Critical volume [m3/kmol] (same as [L/mol])
        static Scalar criticalVolume() {return 6.0976e-1; }
};

template <class Scalar>
class Methane : public Opm::Component<Scalar, Methane<Scalar> >
{
public:
        /// Chemical name
        static const char* name() { return "CH4"; }

        /// Molar mass in \f$\mathrm{[kg/mol]}\f$
        static Scalar molarMass() { return 0.0160; }

        /// Critical temperature in \f$\mathrm[K]}\f$
        static Scalar criticalTemperature() { return 190.5640; }

        /// Critical pressure in \f$\mathrm[Pa]}\f$
        static Scalar criticalPressure() { return 4.599e6; }

        /// Acentric factor
        static Scalar acentricFactor() { return 0.0114; }

        // Critical volume [m3/kmol]
        static Scalar criticalVolume() {return 9.8628e-2; }
};


template <class Scalar>
class Hydrogen : public Opm::Component<Scalar, Hydrogen<Scalar> >
{
public:
         /// Chemical name
        static const char* name() { return "H2"; }

        /// Molar mass in \f$\mathrm{[kg/mol]}\f$
        static Scalar molarMass() { return 0.0020156; }

        /// Critical temperature in \f$\mathrm[K]}\f$
        static Scalar criticalTemperature() { return 33.2; }

        /// Critical pressure in \f$\mathrm[Pa]}\f$
        static Scalar criticalPressure() { return 1.297e6; }

        /// Acentric factor
        static Scalar acentricFactor() { return -0.22; }

        // Critical volume [m3/kmol]
        static Scalar criticalVolume() {return 6.45e-2; }

};

template <class Scalar>
class Nitrogen : public Opm::Component<Scalar, Nitrogen<Scalar> >
{
public:
         /// Chemical name
        static const char* name() { return "N2"; }

        /// Molar mass in \f$\mathrm{[kg/mol]}\f$
        static Scalar molarMass() { return 0.0280134; }

        /// Critical temperature in \f$\mathrm[K]}\f$
        static Scalar criticalTemperature() { return 126.192; }

        /// Critical pressure in \f$\mathrm[Pa]}\f$
        static Scalar criticalPressure() { return 3.3958e6; }

        /// Acentric factor
        static Scalar acentricFactor() { return 0.039; }

        // Critical volume [m3/kmol]
        static Scalar criticalVolume() {return 8.94e-2; }
};

template <class Scalar>
class Water : public Opm::Component<Scalar, Water<Scalar> >
{
public:
         /// Chemical name
        static const char* name() { return "H20"; }

        /// Molar mass in \f$\mathrm{[kg/mol]}\f$
        static Scalar molarMass() { return 0.01801528; }

        /// Critical temperature in \f$\mathrm[K]}\f$
        static Scalar criticalTemperature() { return 647; }

        /// Critical pressure in \f$\mathrm[Pa]}\f$
        static Scalar criticalPressure() { return 22.064e6; }

        /// Acentric factor
        static Scalar acentricFactor() { return 0.344; }

        // Critical volume [m3/kmol]
        static Scalar criticalVolume() {return 5.595e-2; }
};

template <class Scalar>
class ChiwomsCO2 : public Opm::SimpleCO2<Scalar>
{
public:
         /// Chemical name
        static const char* name() { return "CO2"; }

        /// Molar mass in \f$\mathrm{[kg/mol]}\f$
        static Scalar molarMass() { return 0.0440095; }

        /// Critical temperature in \f$\mathrm[K]}\f$
        static Scalar criticalTemperature() { return 304.1; }

        /// Critical pressure in \f$\mathrm[Pa]}\f$
        static Scalar criticalPressure() { return 7.38e6; }

        /// Acentric factor
        static Scalar acentricFactor() { return 0.225; }

        // Critical volume [m3/kmol]
        static Scalar criticalVolume() {return 9.4118e-2; }
};

template <class Scalar>
class ChiwomsBrine : public Opm::H2O<Scalar>
{
public:
        /// Chemical name
        static const char* name() { return "H20-NaCl"; }

        /// Molar mass in \f$\mathrm{[kg/mol]}\f$
        static Scalar molarMass() { return 0.0180158; }

        /// Critical temperature in \f$\mathrm[K]}\f$
        static Scalar criticalTemperature() { return 647.096; }

        /// Critical pressure in \f$\mathrm[Pa]}\f$
        static Scalar criticalPressure() { return 2.21e7; }

        /// Acentric factor
        static Scalar acentricFactor() { return 0.344; }

        // Critical volume [m3/kmol]
        static Scalar criticalVolume() {return 5.595e-2; }
};

struct EOS
{
        template<typename LhsEval>
        static LhsEval oleic_enthalpy(LhsEval T, LhsEval p, LhsEval x) {
                return 0;
        }

        template<typename LhsEval>
        static LhsEval aqueous_enthalpy(LhsEval T, LhsEval p, LhsEval x) {
                return 0;
        }
};

} // namespace opm

#endif // COMPONENTS_HH
