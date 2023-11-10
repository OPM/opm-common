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

#ifndef WDFAC_HPP_HEADER_INCLUDED
#define WDFAC_HPP_HEADER_INCLUDED

namespace Opm {
    class Connection;
    class DeckRecord;
    class WellConnections;
} // namespace Opm

namespace Opm { namespace RestartIO {
    struct RstWell;
}} // namespace Opm::RestartIO

namespace Opm {

    class WDFAC
    {
    public:
        /// Parameters for Dake's D-factor correlation model.
        ///
        /// In particular, holds the coefficient 'A' and the exponents 'B'
        /// and 'C' of the correlation relation
        ///
        ///   D = A * (Ke/K0)**B * porosity**C * Ke / (h * rw) * (sg_g/mu_g)
        ///
        /// in which
        ///
        ///   * Ke is the connection's effective permeability (sqrt(Kx*Ky)
        ///     in the case of a vertical connection)
        ///
        ///   * K0 is a reference/background permeability scale (1mD)
        ///
        ///   * h is the effective length of the connection's perforation
        ///     interval (dz*ntg in the case of a vertical connection)
        ///
        ///   * rw is the connection's wellbore radius
        ///
        ///   * sg_g is the specific gravity of surface condition gas
        ///     relative to surface condition air
        ///
        ///   * mu_g is the reservoir condition viscosity of the free gas phase.
        struct Correlation
        {
            /// Multiplicative coefficient 'A'.
            double coeff_a{0.0};

            /// Power coefficient 'B' for the effective permeability.
            double exponent_b{0.0};

            /// Power coefficient 'C' for the porosity term.
            double exponent_c{0.0};

            /// Serialisation test object.
            static Correlation serializationTestObject();

            /// Equality operator
            ///
            /// \param[in] other Object to which \c *this will be compared.
            bool operator==(const Correlation& other) const;

            /// Inequality operator
            ///
            /// \param[in] other Object to which \c *this will be compared.
            bool operator!=(const Correlation& other) const
            {
                return ! (*this == other);
            }

            /// Serialisation operator
            ///
            /// \tparam Serializer Protocol for serialising and
            ///   deserialising objects between memory and character
            ///   buffers.
            ///
            /// \param[in,out] serializer Serialisation object.
            template <class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(this->coeff_a);
                serializer(this->exponent_b);
                serializer(this->exponent_c);
            }
        };

        /// Serialisation test object
        static WDFAC serializationTestObject();

        double getDFactor(const Connection& connection, double mu, double rho, double phi) const;

        /// Configure D-factor calculation from well-level D-factor
        /// description (keyword 'WDFAC')
        ///
        /// \param[in] record Well-level D-factor description.  Single
        ///   record from WDFAC keyword.
        void updateWDFAC(const DeckRecord& record);

        /// Configure D-factor calculation from Dake correlation model
        /// (keyword WDFACCOR).
        ///
        /// \param[in] record Dake correlation model description.  Single
        ///   record from WDFACCOR keyword.
        void updateWDFACCOR(const DeckRecord& record);

        /// Check if any input-level connctions have a non-trivial D-factor
        /// and update this well's D-factor category accordingly.
        ///
        /// \param[in] connections Connection set as defined by keyword
        ///   COMPDAT.  This function will detect if any of the connections
        ///   created from COMPDAT define a non-trivial D-factor at the
        ///   connection level (item 12 of COMPDAT) and update the D-factor
        ///   category if so.
        void updateWDFACType(const WellConnections& connections);

        /// Capture sum of all CTFs for the purpose of translating
        /// well-level D-factors to connection-level D-factors.
        ///
        /// \param[in] connections Connection set as defined by keyword
        ///   COMPDAT.
        void updateTotalCF(const WellConnections& connections);

        /// Retrieve current D-factor correlation model coefficients.
        const Correlation& getDFactorCorrelationCoefficients() const
        {
            return this->m_corr;
        }

        /// Whether or not a flow-dependent skin factor ('D') has been
        /// configured for the current well.
        bool useDFactor() const;

        /// Equality operator
        ///
        /// \param[in] other Object to which \c *this will be compared.
        bool operator==(const WDFAC& other) const;

        /// Inequality operator
        ///
        /// \param[in] other Object to which \c *this will be compared.
        bool operator!=(const WDFAC& other) const
        {
            return ! (*this == other);
        }

        /// Serialisation operator
        ///
        /// \tparam Serializer Protocol for serialising and deserialising
        ///   objects between memory and character buffers.
        ///
        /// \param[in,out] serializer Serialisation object.
        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->m_type);
            serializer(this->m_d);
            serializer(this->m_total_cf);
            serializer(this->m_corr);
        }

    private:
        /// D-factor categories.
        enum class WDFacType
        {
            /// No flow-dependent skin factor is configured for this well.
            NONE = 1,

            /// Well-level D-factor.
            DFACTOR = 2,

            /// Use Dake's D-factor correlation model.
            DAKEMODEL = 3,

            /// Connection-level D-factor.
            CON_DFACTOR = 4,
        };

        /// D-factor category for this well.
        WDFacType m_type { WDFacType::NONE };

        /// Well-level D-factor for this well.
        double m_d{0.0};

	/// Total CTF sum for this well.
        double m_total_cf{-1.0};

        /// Coefficients for Dake's correlation model.
        Correlation m_corr{};
    };

} // namespace Opm

#endif  // WDFAC_HPP_HEADER_INCLUDED
