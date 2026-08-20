/*
  Copyright 2026 NORCE.

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
#ifndef OPM_SALTARRAY_HPP
#define OPM_SALTARRAY_HPP

#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/material/components/CaIon.hpp>
#include <opm/material/components/ClIon.hpp>
#include <opm/material/components/KIon.hpp>
#include <opm/material/components/MgIon.hpp>
#include <opm/material/components/NaIon.hpp>
#include <opm/material/components/SO4Ion.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <numeric>

namespace Opm
{

// SaltUnit tags for SaltArray
struct SaltMolality
{
};

struct SaltMassFraction
{
};

struct SaltMoleFraction
{
};

// Helper conversion struct
template <class FromSaltUnit, class ToSaltUnit>
struct SaltUnitConverter;

/// Salt ion indices for SaltArray
enum class SaltIndex : std::size_t
{
    NA, K, CA, MG, CL, SO4,
    NCOMP // DO NOT CHANGE! Must be the last enum to set size of SaltArray!
};

/// Enum for classifying cations and anions
enum class IonType
{
    Cation, Anion
};

/*!
 * Converts ion string name to index
 *
 * @param s Ion name
 * @return Index of salt ion
 */
inline SaltIndex
saltIndexFromString(const std::string& s)
{
    if (s == "NA") {
        return SaltIndex::NA;
    }
    if (s == "K") {
        return SaltIndex::K;
    }
    if (s == "CA") {
        return SaltIndex::CA;
    }
    if (s == "MG") {
        return SaltIndex::MG;
    }
    if (s == "CL") {
        return SaltIndex::CL;
    }
    if (s == "SO4") {
        return SaltIndex::SO4;
    }
    throw std::invalid_argument("SaltIndex not valid!");
}

/*!
 * Categorize ion in cation or anion
 *
 * @param s Index of salt ion
 * @return Cation or anion category
 */
inline IonType
categorizeIon(const SaltIndex s)
{
    switch (s) {
    case SaltIndex::CA:
    case SaltIndex::NA:
    case SaltIndex::MG:
    case SaltIndex::K:
        return IonType::Cation;
    case SaltIndex::CL:
    case SaltIndex::SO4:
        return IonType::Anion;
    default:
        throw std::runtime_error("Unknown SaltIndex");
    }
}

/*!
 * Returns the ion strength or salt ion. Positive for cations; negative for anions.
 *
 * @param s Index of salt ion
 * @return Ion strength
 */
inline short
ionStrength(const SaltIndex s)
{
    switch (s) {
    case SaltIndex::CA:
        return 2;
    case SaltIndex::NA:
        return 1;
    case SaltIndex::MG:
        return 2;
    case SaltIndex::K:
        return 1;
    case SaltIndex::CL:
        return -1;
    case SaltIndex::SO4:
        return -2;
    default:
        throw std::runtime_error("Unknown SaltIndex");
    }
}

/*!
 * Get molar mass of salt ion
 *
 * @param ind Index of salt ion
 * @return Molar mass
 */
template <class Scalar>
Scalar
saltMolarMass(const SaltIndex ind)
{
    switch (ind) {
    case SaltIndex::NA:
        return NaIon<Scalar>::molarMass();
    case SaltIndex::K:
        return KIon<Scalar>::molarMass();
    case SaltIndex::CA:
        return CaIon<Scalar>::molarMass();
    case SaltIndex::CL:
        return ClIon<Scalar>::molarMass();
    case SaltIndex::MG:
        return MgIon<Scalar>::molarMass();
    case SaltIndex::SO4:
        return SO4Ion<Scalar>::molarMass();
    default:
        throw std::invalid_argument("SaltIndex not valid!");
    }
}

/*!
 * Wrapper around std::array for salt components with some convenience functions and conversion
 * to SaltArrays with different units
 *
 * @tparam T Value type (typically Scalar or Evaluation)
 * @tparam SaltUnit Unit of salt array elements
 */
template <class T, class SaltUnit>
class SaltArray
{
public:
    using SaltIndexPair = std::pair<std::vector<SaltIndex>, std::vector<SaltIndex> >;

    static constexpr std::size_t ncomp = static_cast<std::size_t>(SaltIndex::NCOMP);

    /*!
     * Default constructor
     */
    SaltArray() = default;

    /*!
     * Conversion constructor
     *
     * @tparam U Value type different from T
     * @param other Salt array to convert from
     */
    template <class U>
    explicit SaltArray(const SaltArray<U, SaltUnit>& other)
    {
        std::ranges::transform(other,
                               saltData_.begin(),
                               [](const U& val) {
                                   return static_cast<T>(val);
                               });
    }

    /*!
     * Assign array elements from a deck array
     *
     * @param record Deck record array
     */
    void assignFromDeckRecord(const DeckRecord& record)
    {
        assert(record.size() == saltData_.size());
        for (const auto& item : record) {
            auto recItem = static_cast<T>(item.get<double>(0));
            auto index = saltIndexFromString(item.name());
            (*this)[index] = recItem;
        }
    }

    /*!
     * Return read-only view of array element at a salt ion index
     *
     * @param ind Index of salt ion
     * @return Array element
     */
    const T& operator[](const SaltIndex ind) const
    {
        return saltData_[static_cast<std::size_t>(ind)];
    }

    /*!
     * Return array element at a salt ion index
     *
     * @param ind Index of salt ion
     * @return Array element
     */
    T& operator[](const SaltIndex ind)
    {
        return saltData_[static_cast<std::size_t>(ind)];
    }

    /*!
     * Check for equal SaltArray
     *
     * @param other Comparison array
     * @return True if arrays are equal
     */
    bool operator==(const SaltArray& other) const
    {
        return saltData_ == other.saltData_;
    }

    /*!
     * Default copy assignment
     *
     * @param other Array to copy
     * @return Copy of other array
     */
    SaltArray& operator=(const SaltArray& other) = default;

    /*!
     * Conversion copy assignment for simple assignment to new value type (that supports conversion)
     *
     * @tparam U Value type of other array
     * @param other Array to copy
     * @return Copy of array
     */
    template <class U>
    SaltArray& operator=(const SaltArray<U, SaltUnit>& other)
    {
        std::ranges::transform(other,
                               saltData_.begin(),
                               [](const U& val) {
                                   return static_cast<T>(val);
                               });
        return *this;
    }

    /*!
     * Array begin iterator
     *
     * @return Iterator
     */
    auto begin()
    {
        return saltData_.begin();
    }

    /*!
     * Array end iterator
     *
     * @return Iterator
     */
    auto end()
    {
        return saltData_.end();
    }

    /*!
     * Read-only view of array begin iterator
     *
     * @return Iterator
     */
    [[nodiscard]] auto begin() const
    {
        return saltData_.begin();
    }

    /*!
     * Read-only view of array end iterator
     *
     * @return Iterator
     */
    [[nodiscard]] auto end() const
    {
        return saltData_.end();
    }

    /*!
     * Size of array, which should be equal to number of supported ions in @ref SaltIndex
     *
     * @return Size of array
     */
    [[nodiscard]] constexpr std::size_t size() const
    {
        return saltData_.size();
    }

    /*!
     * Sum of array elements.
     * @note For mass and mole fraction arrays, 1 - sum() is equal to mass/mole fraction of H2O
     *
     * @return Sum
     */
    T sum() const
    {
        return std::accumulate(begin(), end(), T{});
    }

    /*!
     * Fill all elements with zero
     */
    void clear() noexcept
    {
        saltData_.fill(T{});
    }

    /*!
     * Check if any elements are non-zero
     *
     * @return True if any element non-zero
     */
    [[nodiscard]] bool any_nonzero() const noexcept
    {
        return std::any_of(begin(),
                           end(),
                           [](const T& val) {
                               return val != T{};
                           });
    }

    /*!
     * Get cations and anions of non-zero ions in descending ion strength
     *
     * @return Pair of cation and anion vectors
     */
    [[nodiscard]] SaltIndexPair cations_and_anions() const
    {
        std::vector<std::pair<short, SaltIndex> > cations;
        std::vector<std::pair<short, SaltIndex> > anions;
        cations.reserve(size());
        anions.reserve(size());
        for (std::size_t i = 0; i < size(); ++i) {
            auto ind = static_cast<SaltIndex>(i);
            auto ionStr = ionStrength(ind);
            if ((*this)[ind] != T{}) {
                if (categorizeIon(ind) == IonType::Cation) {
                    cations.emplace_back(ionStr, ind);

                } else {
                    anions.emplace_back(ionStr, ind);
                }
            }
        }

        // Sort by ion strength: descending for cations, ascending for anions
        std::ranges::sort(cations, std::ranges::greater{}, &std::pair<short, SaltIndex>::first);
        std::ranges::sort(anions, {}, &std::pair<short, SaltIndex>::first);

        // Return only cations and anions SaltIndex
        std::vector<SaltIndex> cationIndex;
        std::vector<SaltIndex> anionIndex;
        cationIndex.reserve(cations.size());
        anionIndex.reserve(anions.size());
        std::ranges::transform(cations,
                               std::back_inserter(cationIndex),
                               &std::pair<short, SaltIndex>::second);
        std::ranges::transform(anions,
                               std::back_inserter(anionIndex),
                               &std::pair<short, SaltIndex>::second);

        return {cationIndex, anionIndex};
    }

    /*!
     * Return an array with elements converted to new unit
     *
     * @tparam NewSaltUnit New unit for returning array
     * @return Converted array
     */
    template <class NewSaltUnit>
    SaltArray<T, NewSaltUnit> convert_to() const
    {
        return SaltUnitConverter<SaltUnit, NewSaltUnit>::template convert<T>(*this);
    }

    template <class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(saltData_);
    }

private:
    std::array<T, ncomp> saltData_{}; ///< Array of salt ion values
}; // class SaltArray

// ------------------------------------------
// Conversion between different salt units
// ------------------------------------------
template <>
struct SaltUnitConverter<SaltMassFraction, SaltMolality>
{
    /*!
     * Convert mass fraction to molality:
     * https://en.wikipedia.org/wiki/Molality#Mass_fraction
     *
     * @tparam T Value type
     * @param saltArray Mass fraction array
     * @return Molality array
     */
    template <class T>
    static SaltArray<T, SaltMolality> convert(const SaltArray<T, SaltMassFraction>& saltArray)
    {
        SaltArray<T, SaltMolality> molalityArray;
        for (std::size_t i = 0; i < saltArray.size(); ++i) {
            auto sIdx = static_cast<SaltIndex>(i);
            molalityArray[sIdx] = saltArray[sIdx] / saltMolarMass<T>(sIdx);
        }

        T s = 1.0 - saltArray.sum();
        for (auto& elem : molalityArray) {
            elem /= s;
        }

        return molalityArray;
    }
};

template <>
struct SaltUnitConverter<SaltMassFraction, SaltMoleFraction>
{
    /*!
     * Convert mass fraction to mole fraction:
     * https://en.wikipedia.org/wiki/Mass_fraction_(chemistry)#Mole_fraction
     *
     * @tparam T Value type
     * @param saltArray Mass fraction array
     * @return Mole fraction array
     */
    template <class T>
    static SaltArray<T, SaltMoleFraction> convert(const SaltArray<T, SaltMassFraction>& saltArray)
    {
        SaltArray<T, SaltMoleFraction> moleFracArray;
        T s = 1.0;
        for (std::size_t i = 0; i < saltArray.size(); ++i) {
            auto sIdx = static_cast<SaltIndex>(i);
            moleFracArray[sIdx] = saltArray[sIdx] * 18.01518e-3 / saltMolarMass<T>(sIdx);
            s += moleFracArray[sIdx] - saltArray[sIdx];
        }

        for (auto& elem : moleFracArray) {
            elem /= s;
        }

        return moleFracArray;
    }
};

template <>
struct SaltUnitConverter<SaltMoleFraction, SaltMassFraction>
{
    /*!
     * Convert mole fraction to mass fraction:
     * https://en.wikipedia.org/wiki/Mole_fraction#Mass_fraction
     *
     * @tparam T Value type
     * @param saltArray Mole fraction array
     * @return Mass fraction array
     */
    template <class T>
    static SaltArray<T, SaltMassFraction> convert(const SaltArray<T, SaltMoleFraction>& saltArray)
    {
        SaltArray<T, SaltMassFraction> massFracArray;
        T s = 18.01518e-3;
        for (std::size_t i = 0; i < saltArray.size(); ++i) {
            auto sIdx = static_cast<SaltIndex>(i);
            massFracArray[sIdx] = saltArray[sIdx] * saltMolarMass<T>(sIdx);
            s += massFracArray[sIdx] - saltArray[sIdx] * 18.01518e-3;
        }

        for (auto& elem : massFracArray) {
            elem /= s;
        }

        return massFracArray;
    }
};

template <>
struct SaltUnitConverter<SaltMoleFraction, SaltMolality>
{
    /*!
     * Convert mole fraction to molality:
     * https://en.wikipedia.org/wiki/Molality#Mole_fraction
     *
     * @tparam T Value type
     * @param saltArray Mole fraction array
     * @return Molality array
     */
    template <class T>
    static SaltArray<T, SaltMolality> convert(const SaltArray<T, SaltMoleFraction>& saltArray)
    {
        SaltArray<T, SaltMolality> molalityArray;
        for (std::size_t i = 0; i < saltArray.size(); ++i) {
            auto sIdx = static_cast<SaltIndex>(i);
            molalityArray[sIdx] = saltArray[sIdx] / 18.01518e-3;
        }

        T s = 1.0 - saltArray.sum();
        for (auto& elem : molalityArray) {
            elem /= s;
        }

        return molalityArray;
    }
};

template <>
struct SaltUnitConverter<SaltMolality, SaltMoleFraction>
{
    /*!
     * Convert molality to mole fraction:
     * https://en.wikipedia.org/wiki/Molality#Mole_fraction
     *
     * @tparam T Value type
     * @param saltArray Molality array
     * @return Mole fraction array
     */
    template <class T>
    static SaltArray<T, SaltMoleFraction> convert(const SaltArray<T, SaltMolality>& saltArray)
    {
        SaltArray<T, SaltMoleFraction> moleFracArray;
        T s = 1.0;
        for (std::size_t i = 0; i < saltArray.size(); ++i) {
            auto sIdx = static_cast<SaltIndex>(i);
            moleFracArray[sIdx] = saltArray[sIdx] * 18.01518e-3;
            s += moleFracArray[sIdx];
        }

        for (auto& elem : moleFracArray) {
            elem /= s;
        }

        return moleFracArray;
    }
};

template <>
struct SaltUnitConverter<SaltMolality, SaltMassFraction>
{
    /*!
     * Convert molality to mass fraction:
     * https://en.wikipedia.org/wiki/Molality#Mass_fraction
     *
     * @tparam T Value type
     * @param saltArray Molality array
     * @return Mass fraction array
     */
    template <class T>
    static SaltArray<T, SaltMassFraction> convert(const SaltArray<T, SaltMolality>& saltArray)
    {
        SaltArray<T, SaltMassFraction> massFracArray;
        T s = 1.0;
        for (std::size_t i = 0; i < saltArray.size(); ++i) {
            auto sIdx = static_cast<SaltIndex>(i);
            massFracArray[sIdx] = saltArray[sIdx] * saltMolarMass<T>(sIdx);
            s += massFracArray[sIdx];
        }

        for (auto& elem : massFracArray) {
            elem /= s;
        }

        return massFracArray;
    }
};

}

#endif //OPM_SALTARRAY_HPP
