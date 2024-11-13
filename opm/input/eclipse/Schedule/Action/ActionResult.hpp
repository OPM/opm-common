/*
  Copyright 2019 Equinor ASA.

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

#ifndef ACTION_RESULT_HPP
#define ACTION_RESULT_HPP

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace Opm::Action {

/// Class Action::Result holds the boolean result of a ACTIONX condition
/// like
///
///    WWCT > 0.75
///
/// In addition to the overall truthness of the expression an Action::Result
/// instance can optionally have a set of matching entities such as wells or
/// groups.  For instance the the result of:
///
///    FWCT > 0.75
///
/// will not contain any wells, whereas the result of:
///
///    WWCT > 0.75
///
/// will contain a set of all the wells matching the condition.  The set of
/// matching entities can be queried with the matches() member function.
/// When a result with matching entities and a result without matching
/// entities are combined, for instance as,
///
///    WWCT > 0.50 AND FPR > 1000
///
/// the result will have all the wells corresponding to the first condition.
/// When multiple results with matching entities are combined with logical
/// operators AND and OR, the set of matching entities are combined
/// accordingly--as a set union for 'OR' and a set intersection for 'AND'.
///
/// It is possible to have a result which evaluates to true and still have
/// zero matching entities
///
///    WWCT > 1.25 OR FOPT > 1
///
/// in which case it is likely that the WWCT condition is false, but the
/// FOPT condition is true.  If the condition evaluates to true the set of
/// matching entities will be passed to the Schedule::applyAction() method,
/// and will be substituted in place of the name pattern '?' in keywords
/// like WELOPEN.

class Result
{
public:
    /// Random access range of values
    ///
    /// Poor-man's substitute for C++20's std::span<T>.
    ///
    /// \tparam T Element type of range.  Typically std::string since the
    /// common use case of class ValueRange<> is to represent a sequence of
    /// well or group names.
    template <typename T>
    class ValueRange
    {
    public:
        /// Random access iterator.
        ///
        /// Class ValueRange<> assumes that the underlying sequence is a
        /// std::vector<T> of sufficient lifetime.
        using RandIt = typename std::vector<T>::const_iterator;

        /// Constructor.
        ///
        /// Forms a ValueRange object from an iterator range.
        ///
        /// \param[in] first First element in value range.
        ///
        /// \param[in] last One past the end of the elements in the value range.
        ///
        /// \param[in] isSorted Wether or not the value range is sorted by
        /// \code std::less<> \endcode.  In that case, element existence can
        /// be established in O(log(n)) time.  This is an optimisation hook.
        explicit ValueRange(RandIt first, RandIt last, bool isSorted = false)
            : first_    { first }
            , last_     { last }
            , isSorted_ { isSorted }
        {}

        /// Beginning of value range's elements.
        auto begin() const { return this->first_; }

        /// End of value range's elements.
        auto end() const { return this->last_; }

        /// Predicate for an empty value range.
        auto empty() const { return this->begin() == this->end(); }

        /// Number of elements in the value range.
        auto size() const { return std::distance(this->begin(), this->end()); }

        /// Convert value range to a std::vector.
        ///
        /// Copies elements.
        std::vector<T> asVector() const
        {
            return { this->begin(), this->end() };
        }

        /// Element existence predicate.
        ///
        /// \param[in] elem Element for which to check existence.
        ///
        /// \return Whether or not \p elem exists in the value range.
        bool hasElement(const T& elem) const
        {
            return this->isSorted_
                ? this->hasElementSorted(elem)
                : this->hasElementUnsorted(elem);
        }

    private:
        /// First element in value range.
        RandIt first_{};

        /// One past the end of the value range's elements.
        RandIt last_{};

        /// Whether or not the value range bounds a sorted sequence.
        bool isSorted_{false};

        /// Element existence predicate for sorted sequences
        ///
        /// \param[in] elem Element for which to check existence.
        ///
        /// \return Whether or not \p elem exists in the value range.
        bool hasElementSorted(const T& elem) const
        {
            return std::binary_search(this->begin(), this->end(), elem);
        }

        /// Element existence predicate for unsorted sequences
        ///
        /// \param[in] elem Element for which to check existence.
        ///
        /// \return Whether or not \p elem exists in the value range.
        bool hasElementUnsorted(const T& elem) const
        {
            return std::find(this->begin(), this->end(), elem)
                != this->end();
        }
    };

    /// Container of matching entities
    ///
    /// These are entities--typically wells or groups--for which an ACTIONX
    /// condition (or sub-condition) holds.
    class MatchingEntities
    {
    public:
        /// Default constructor.
        ///
        /// Forms an empty set of matching entities.  Expected to be called
        /// by class Result only.
        MatchingEntities();

        /// Copy constructor.
        ///
        /// \param[in] rhs Source object from which to form an independent
        /// copy.
        MatchingEntities(const MatchingEntities& rhs);

        /// Move constructor.
        ///
        /// \param[in,out] rhs Source object from which form a new object.
        /// Left in an empty state on return from the move constructor.
        MatchingEntities(MatchingEntities&& rhs);

        /// Destructor.
        ///
        /// Needed for PIMPL idiom.
        ~MatchingEntities();

        /// Assignment operator.
        ///
        /// \param[in] rhs Source object whose value will overwrite \code
        /// *this.
        ///
        /// \return *this.
        MatchingEntities& operator=(const MatchingEntities& rhs);

        /// Move-assignment operator.
        ///
        /// \param[in] rhs Source object which will be moved into \code
        /// *this.  Left in an empty state on return from the move
        /// constructor.
        ///
        /// \return *this.
        MatchingEntities& operator=(MatchingEntities&& rhs);

        /// Get sequence of read-only well names for which the current
        /// Result's conditionSatisfied() member function returns true.
        ValueRange<std::string> wells() const;

        /// Whether or not named well is in the list of matching entities.
        ///
        /// \param[in] well Well name.
        ///
        /// \return Whether or not \p well is among the matching wells.
        bool hasWell(const std::string& well) const;

        /// Equality predicate.
        ///
        /// \param[in] that Object against which \code *this \endcode will
        /// be tested for equality.
        ///
        /// \return Whether or not \code *this \endcode is the same as \p
        /// that.
        bool operator==(const MatchingEntities& that) const;

        friend class Result;

    private:
        /// Implementation class
        class Impl;

        /// Pointer to implementation.
        std::unique_ptr<Impl> pImpl_;

        /// Add a well name to the set of matching entities.
        ///
        /// Primarily for use by class Result.
        ///
        /// \param[in] well Well name that will be included in set of
        /// matching entities.
        void addWell(const std::string& well);

        /// Add a sequence of well names to the set of matching entities.
        ///
        /// Primarily for use by class Result.
        ///
        /// \param[in] wells Sequence of well names that will be included in
        /// set of matching entities.
        void addWells(const std::vector<std::string>& wells);

        /// Incorporate a set of matching entities into current set as if by
        /// set intersection.
        ///
        /// Primarily for use by class Result.  Implements the 'AND'
        /// conjunction of ACTIONX conditions.
        ///
        /// \param[in] rhs Other set of matching enties.
        void makeIntersection(const MatchingEntities& rhs);

        /// Incorporate a set of matching entities into current set as if by
        /// set union.
        ///
        /// Primarily for use by class Result.  Implements the 'OR'
        /// disjunction of ACTIONX conditions.
        ///
        /// \param[in] rhs Other set of matching enties.
        void makeUnion(const MatchingEntities& rhs);

        /// Remove all matching entities from internal storage.
        ///
        /// Primarily for use by class Result.
        void clear();
    };

    /// Constructor.
    ///
    /// Creates a result set with a known condition value.
    ///
    /// \param[in] result_arg Condition value.
    explicit Result(bool result_arg);

    /// Copy constructor.
    ///
    /// \param[in] rhs Source object from which to form an independent copy.
    Result(const Result& rhs);

    /// Move constructor.
    ///
    /// \param[in,out] rhs Source object from which form a new object.  Left
    /// in an empty state on return from the move constructor.
    Result(Result&& rhs);

    /// Destructor.
    ///
    /// Needed for PIMPL idiom.
    ~Result();

    /// Assignment operator.
    ///
    /// \param[in] rhs Source object whose value will overwrite \code *this.
    ///
    /// \return *this.
    Result& operator=(const Result& rhs);

    /// Move-assignment operator.
    ///
    /// \param[in] rhs Source object which will be moved into \code *this.
    /// Left in an empty state on return from the move constructor.
    ///
    /// \return *this.
    Result& operator=(Result&& rhs);

    /// Include a sequence of well names as matching entities.
    ///
    /// Duplicates, if any, will be ignored.
    ///
    /// \param[in] w Sequence of well names.
    ///
    /// \return *this.
    Result& wells(const std::vector<std::string>& w);

    /// Predicate for whether or not the result set represents a 'true'
    /// value.
    ///
    /// \return Result set's condition value--i.e., constructor argument.
    bool conditionSatisfied() const;

    /// Incorporate another result set into the current set as if by set
    /// union.
    ///
    /// Final result set condition value is logical OR of the current
    /// condition value and that of the other result set.  Set of matching
    /// entities is union of the current set and that of the other result
    /// set.
    ///
    /// \param[in] rhs Other result set.
    ///
    /// \return \code *this \endcode after the \p rhs result set has been
    /// incorporated.
    Result& makeSetUnion(const Result& rhs);

    /// Incorporate another result set into the current set as if by set
    /// intersection.
    ///
    /// Final result set condition value is logical AND of the current
    /// condition value and that of the other result set.  Set of matching
    /// entities is the intersection of the current set and that of the
    /// other result set, albeit with special case handling of empty sets of
    /// matching entities.
    ///
    /// \param[in] rhs Other result set.
    ///
    /// \return \code *this \endcode after the \p rhs result set has been
    /// incorporated.
    Result& makeSetIntersection(const Result& rhs);

    /// Retrieve set of matching entities.
    ///
    /// Empty unless conditionSatisfied() is true.
    ///
    /// \return Set of matching entities.
    const MatchingEntities& matches() const;

    /// Equality predicate.
    ///
    /// \param[in] that Object against which \code *this \endcode will be
    /// tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p that.
    bool operator==(const Result& that) const;

private:
    /// Implementation class.
    class Impl;

    /// Pointer to implementation.
    std::unique_ptr<Impl> pImpl_{};
};

} // Namespace Opm::Action

#endif // ACTION_RESULT_HPP
