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

#include <opm/input/eclipse/Schedule/Action/ActionResult.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

/// Set implementation on top of a sorted vector
///
/// \tparam T Element type.  Often std::string, but other types are
/// supported.
template <typename T>
class SortedVectorSet
{
public:
    /// Insert sequence of elements
    ///
    /// \tparam InputIt Input iterator type.  Must be dereferenceable and
    /// the resulting type must be convertible to \c T.
    ///
    /// \param[in] first First element in input element range.
    ///
    /// \param[in] last One past the end of the elements in the input range.
    template <typename InputIt>
    void insert(InputIt first, InputIt last);

    /// Insert single element by copy.
    ///
    /// \param[in] elem Element.
    void insert(const T& elem);

    /// Insert single element by move.
    ///
    /// \param[in] elem Element.
    void insert(T&& elem);

    /// Remove all set elements from internal storage.
    void clear();

    /// Form a proper set by sorting current elements and removing
    /// duplicates.
    ///
    /// Element sorted by std::less<> and element equality determined by
    /// std::equal_to<>.
    void commit();

    /// Element existence predicate.
    ///
    /// Equivalent elements determined by std::less<>.
    ///
    /// Client code must call commit() prior to testing for element
    /// existence.
    ///
    /// \param[in] elem Element for which to check existence.
    ///
    /// \return Whether or not \p elem exists in the set range.
    bool hasElement(const T& elem) const;

    /// Incorporate another result set into the current set as if by set
    /// intersection.
    ///
    /// Elements ordered by std::less<>.
    ///
    /// \param[in] rhs Other result set.
    void makeIntersection(const SortedVectorSet& rhs);

    /// Incorporate another result set into the current set as if by set
    /// union.
    ///
    /// Elements ordered by std::less<>.
    ///
    /// \param[in] rhs Other set.
    void makeUnion(const SortedVectorSet& rhs);

    /// Form a proper set by sorting current elements and removing
    /// duplicates.
    ///
    /// \tparam Compare Element comparison operation.
    ///
    /// \tparam Equal Element equality predicate.
    ///
    /// \param[in] cmp Element comparison.
    ///
    /// \param[in] eq Element equality.
    template <typename Compare, typename Equal>
    void commit(Compare&& cmp, Equal&& eq);

    /// Element existence predicate.
    ///
    /// Equivalent elements determined by custom comparator.
    ///
    /// Client code must call commit() prior to testing for element
    /// existence.
    ///
    /// \tparam Compare Element comparison operation.
    ///
    /// \param[in] elem Element for which to check existence.
    ///
    /// \param[in] cmp Element comparison.
    ///
    /// \return Whether or not \p elem exists in the set range ordered by \p
    /// cmp.
    template <typename Compare>
    bool hasElement(const T& elem, Compare&& cmp) const;

    /// Incorporate another result set into the current set as if by set
    /// intersection.
    ///
    /// \tparam Compare Element comparison operation.
    ///
    /// \param[in] rhs Other set.
    ///
    /// \param[in] cmp Element comparison.
    template <typename Compare>
    void makeIntersection(const SortedVectorSet& rhs, Compare&& cmp);

    /// Incorporate another result set into the current set as if by set
    /// union.
    ///
    /// \tparam Compare Element comparison operation.
    ///
    /// \param[in] rhs Other set.
    ///
    /// \param[in] cmp Element comparison.
    template <typename Compare>
    void makeUnion(const SortedVectorSet& rhs, Compare&& cmp);

    /// Beginning of set's elements.
    auto begin() const { return this->elems_.begin(); }

    /// End of set's elements.
    auto end() const { return this->elems_.end(); }

    /// Predicate for an empty set.
    bool empty() const
    {
        return this->elems_.empty();
    }

    /// Equality predicate.
    ///
    /// \param[in] data Object against which \code *this \endcode will
    /// be tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p data.
    bool operator==(const SortedVectorSet& that) const
    {
        return this->elems_ == that.elems_;
    }

private:
    /// Set elements.
    std::vector<T> elems_{};
};

template <typename T>
template <typename InputIt>
void SortedVectorSet<T>::insert(InputIt first, InputIt last)
{
    this->elems_.insert(this->elems_.end(), first, last);
}

template <typename T>
void SortedVectorSet<T>::insert(const T& elem)
{
    this->elems_.push_back(elem);
}

template <typename T>
void SortedVectorSet<T>::insert(T&& elem)
{
    this->elems_.push_back(std::move(elem));
}

template <typename T>
void SortedVectorSet<T>::clear()
{
    this->elems_.clear();
}

template <typename T>
void SortedVectorSet<T>::commit()
{
    this->commit(std::less<>{}, std::equal_to<>{});
}

template <typename T>
bool SortedVectorSet<T>::hasElement(const T& elem) const
{
    return this->hasElement(elem, std::less<>{});
}

template <typename T>
void SortedVectorSet<T>::makeIntersection(const SortedVectorSet& rhs)
{
    this->makeIntersection(rhs, std::less<>{});
}

template <typename T>
void SortedVectorSet<T>::makeUnion(const SortedVectorSet& rhs)
{
    this->makeUnion(rhs, std::less<>{});
}

template <typename T>
template <typename Compare, typename Equivalent>
void SortedVectorSet<T>::commit(Compare&& cmp, Equivalent&& eq)
{
    auto i = std::vector<typename std::vector<T>::size_type>(this->elems_.size());
    std::iota(i.begin(), i.end(), typename std::vector<T>::size_type{});

    std::ranges::sort(i,
                      [this, cmp](const auto& i1, const auto& i2)
                      { return cmp(this->elems_[i1], this->elems_[i2]); });

    auto u = std::unique(i.begin(), i.end(), [this, eq](const auto& i1, const auto& i2)
    {
        return eq(this->elems_[i1], this->elems_[i2]);
    });

    i.erase(u, i.end());

    auto unique_sorted_elems = std::vector<T>{};
    unique_sorted_elems.reserve(i.size());

    std::ranges::transform(i, std::back_inserter(unique_sorted_elems),
                           [this](const auto& ix) { return std::move(this->elems_[ix]); });

    this->elems_.swap(unique_sorted_elems);
}

template <typename T>
template <typename Compare>
bool SortedVectorSet<T>::hasElement(const T& elem, Compare&& cmp) const
{
    return std::binary_search(this->elems_.begin(), this->elems_.end(),
                              elem, std::forward<Compare>(cmp));
}

template <typename T>
template <typename Compare>
void SortedVectorSet<T>::makeIntersection(const SortedVectorSet& rhs, Compare&& cmp)
{
    auto i = std::vector<T>{};
    i.reserve(std::min(this->elems_.size(), rhs.elems_.size()));

    std::set_intersection(this->elems_.begin(), this->elems_.end(),
                          rhs  .elems_.begin(), rhs  .elems_.end(),
                          std::back_inserter(i),
                          std::forward<Compare>(cmp));

    this->elems_.swap(i);
}

template <typename T>
template <typename Compare>
void SortedVectorSet<T>::makeUnion(const SortedVectorSet& rhs, Compare&& cmp)
{
    auto u = std::vector<T>{};
    u.reserve(this->elems_.size() + rhs.elems_.size());

    std::set_union(this->elems_.begin(), this->elems_.end(),
                   rhs  .elems_.begin(), rhs  .elems_.end(),
                   std::back_inserter(u),
                   std::forward<Compare>(cmp));

    this->elems_.swap(u);
}

/// Special case set intersection handling for potentially empty sets.
///
/// This is mainly to support intersecting a scalar result set (no matching
/// entities) with a non-scalar result set (with matching entities) and not
/// have the resultant set come out empty.  While not a true intersection
/// operation, this behaviour is useful for ACTIONX processing.
///
/// Note: This is not a member function of SortedVectorSet because it does
/// not need to be.
///
/// \tparam T Set element type.
///
/// \param[in] other Result set.
///
/// \param[in,out] curr Current result set.  \p other will be incorporated
/// into 'curr' while paying attention to which, if any, of \p other or \p
/// curr may be nullopt.
template <typename T>
void intersectWithEmptyHandling(const std::optional<SortedVectorSet<T>>& other,
                                std::optional<SortedVectorSet<T>>&       curr)
{
    // If 'other' does not have a value (i.e., is nullopt), then the result
    // set (curr) should remain unchanged.  Otherwise, if 'curr' does not
    // have a value, then the result set should be 'other'.  Otherwise, when
    // both 'curr' and 'other' have values, the result set should be the set
    // intersection of 'curr' and 'other'.
    //
    // These rules permit intersecting the match set of a scalar condition,
    // e.g., a condition that compares a field-level quantity to a scalar,
    // with a well (or group) condition, for instance something like
    //
    //    FGOR > 432.1 AND /
    //    WOPR 'OP*' <= 123.4 /
    //
    // and having the result set match that of the WOPR condition.

    if (! other.has_value()) {
        return;
    }

    if (! curr.has_value()) {
        curr = other;
    }
    else {
        curr->makeIntersection(*other);
    }
}

} // Anonymous namespace

// ===========================================================================

/// Implementation of Result::MatchingEntities
class Opm::Action::Result::MatchingEntities::Impl
{
public:
    /// Whether or not named well is in the list of matching entities.
    ///
    /// \param[in] well Well name.
    ///
    /// \return Whether or not \p well is among the matching wells.
    bool hasWell(const std::string& well) const;

    /// Get sequence of read-only well names.
    ValueRange<std::string> wells() const;

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
    /// \param[in] wnames Sequence of well names that will be included in
    /// set of matching entities.
    void addWells(const std::vector<std::string>& wnames);

    /// Remove all matching entities from internal storage.
    void clear();

    /// Incorporate a set of matching entities into current set as if by set
    /// intersection.
    ///
    /// Implements the 'AND' conjunction of ACTIONX conditions.
    ///
    /// \param[in] rhs Other set of matching enties.
    void makeIntersection(const Impl& rhs);

    /// Incorporate a set of matching entities into current set as if by set
    /// union.
    ///
    /// Implements the 'OR' disjunction of ACTIONX conditions.
    ///
    /// \param[in] rhs Other set of matching enties.
    void makeUnion(const Impl& rhs);

    /// Equality predicate.
    ///
    /// \param[in] that Object against which \code *this \endcode will
    /// be tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p that.
    bool operator==(const Impl& that) const;

private:
    /// Set of matching wells.
    std::optional<SortedVectorSet<std::string>> wells_{};
    std::vector<std::string> empty_{};

    void ensureWellSetExists();
};

bool Opm::Action::Result::MatchingEntities::Impl::
hasWell(const std::string& well) const
{
    return this->wells_.has_value()
        && this->wells_->hasElement(well);
}

Opm::Action::Result::ValueRange<std::string>
Opm::Action::Result::MatchingEntities::Impl::wells() const
{
    if (! this->wells_.has_value()) {
        return ValueRange<std::string> {
            this->empty_.begin(), this->empty_.end()
        };
    }

    return ValueRange<std::string> {
        this->wells_->begin(), this->wells_->end(),
        /* isSorted = */ true
    };
}

void Opm::Action::Result::MatchingEntities::Impl::
addWell(const std::string& wname)
{
    this->ensureWellSetExists();

    this->wells_->insert(wname);
    this->wells_->commit();
}

void Opm::Action::Result::MatchingEntities::Impl::
addWells(const std::vector<std::string>& wnames)
{
    this->ensureWellSetExists();

    this->wells_->insert(wnames.begin(), wnames.end());
    this->wells_->commit();
}

void Opm::Action::Result::MatchingEntities::Impl::clear()
{
    if (! this->wells_.has_value()) {
        return;
    }

    this->wells_->clear();
}

void Opm::Action::Result::MatchingEntities::Impl::makeIntersection(const Impl& rhs)
{
    intersectWithEmptyHandling(rhs.wells_, this->wells_);
}

void Opm::Action::Result::MatchingEntities::Impl::makeUnion(const Impl& rhs)
{
    if (! rhs.wells_.has_value()) {
        return;
    }

    this->ensureWellSetExists();

    this->wells_->makeUnion(*rhs.wells_);
}

bool Opm::Action::Result::MatchingEntities::Impl::operator==(const Impl& that) const
{
    return this->wells_ == that.wells_;
}

void Opm::Action::Result::MatchingEntities::Impl::ensureWellSetExists()
{
    if (! this->wells_.has_value()) {
        this->wells_ = SortedVectorSet<std::string>{};
    }
}

// ---------------------------------------------------------------------------

Opm::Action::Result::MatchingEntities::MatchingEntities()
    : pImpl_ { std::make_unique<Impl>() }
{}

Opm::Action::Result::MatchingEntities::
MatchingEntities(const MatchingEntities& rhs)
    : pImpl_ { std::make_unique<Impl>(*rhs.pImpl_) }
{}

Opm::Action::Result::MatchingEntities::MatchingEntities(MatchingEntities&& rhs)
    : pImpl_ { std::move(rhs.pImpl_) }
{}

Opm::Action::Result::MatchingEntities::~MatchingEntities() = default;

Opm::Action::Result::MatchingEntities&
Opm::Action::Result::MatchingEntities::operator=(const MatchingEntities& rhs)
{
    this->pImpl_ = std::make_unique<Impl>(*rhs.pImpl_);
    return *this;
}

Opm::Action::Result::MatchingEntities&
Opm::Action::Result::MatchingEntities::operator=(MatchingEntities&& rhs)
{
    this->pImpl_ = std::move(rhs.pImpl_);
    return *this;
}

Opm::Action::Result::ValueRange<std::string>
Opm::Action::Result::MatchingEntities::wells() const
{
    return this->pImpl_->wells();
}

bool Opm::Action::Result::MatchingEntities::hasWell(const std::string& well) const
{
    return this->pImpl_->hasWell(well);
}

bool Opm::Action::Result::MatchingEntities::operator==(const MatchingEntities& that) const
{
    return *this->pImpl_ == *that.pImpl_;
}

void Opm::Action::Result::MatchingEntities::addWell(const std::string& well)
{
    this->pImpl_->addWell(well);
}

void Opm::Action::Result::MatchingEntities::
addWells(const std::vector<std::string>& wells)
{
    this->pImpl_->addWells(wells);
}

void Opm::Action::Result::MatchingEntities::makeIntersection(const MatchingEntities& rhs)
{
    this->pImpl_->makeIntersection(*rhs.pImpl_);
}

void Opm::Action::Result::MatchingEntities::makeUnion(const MatchingEntities& rhs)
{
    this->pImpl_->makeUnion(*rhs.pImpl_);
}

void Opm::Action::Result::MatchingEntities::clear()
{
    this->pImpl_->clear();
}

// ===========================================================================

/// Implementation of Action::Result
class Opm::Action::Result::Impl
{
public:
    /// Constructor.
    ///
    /// Creates a result set with a known condition value.
    ///
    /// \param[in] result Condition value.
    explicit Impl(const bool result) : result_ { result } {}

    /// Get read/write access to set of matching entities.
    MatchingEntities& matches() { return this->matches_; }

    /// Get read only access to set of matching entities.
    const MatchingEntities& matches() const { return this->matches_; }

    /// Get condition value.
    bool result() const { return this->result_; }

    /// Incorporate another result set into the current set as if by set
    /// union.
    ///
    /// Final result set condition value is logical OR of the current
    /// condition value and that of the other result set.  Set of matching
    /// entities is union of the current set and that of the other result
    /// set.
    ///
    /// Any existing set of matching entities will be cleared if the
    /// aggregate condition value is false.
    ///
    /// \param[in] rhs Other result set.
    void makeSetUnion(const Impl& rhs);

    /// Incorporate another result set into the current set as if by set
    /// intersection.
    ///
    /// Final result set condition value is logical AND of the current
    /// condition value and that of the other result set.  Set of matching
    /// entities is the intersection of the current set and that of the
    /// other result set, albeit with special case handling of empty sets of
    /// matching entities.
    ///
    /// Any existing set of matching entities will be cleared if the
    /// aggregate condition value is false.
    ///
    /// \param[in] rhs Other result set.
    void makeSetIntersection(const Impl& rhs);

    /// Equality predicate.
    ///
    /// \param[in] that Object against which \code *this \endcode will be
    /// tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p that.
    bool operator==(const Impl& that) const;

private:
    /// Condition value.
    bool result_{false};

    /// Set of matching entities.
    MatchingEntities matches_{};
};

void Opm::Action::Result::Impl::makeSetUnion(const Impl& rhs)
{
    this->result_ = this->result_ || rhs.result_;

    if (! this->result_) {
        this->matches_.clear();
    }
    else {
        this->matches_.makeUnion(rhs.matches());
    }
}

void Opm::Action::Result::Impl::makeSetIntersection(const Impl& rhs)
{
    this->result_ = this->result_ && rhs.result_;

    if (! this->result_) {
        this->matches_.clear();
    }
    else {
        this->matches_.makeIntersection(rhs.matches());
    }
}

bool Opm::Action::Result::Impl::operator==(const Impl& that) const
{
    return (this->result_  == that.result_)
        && (this->matches_ == that.matches_);
}

// ---------------------------------------------------------------------------

Opm::Action::Result::Result(const bool result)
    : pImpl_ { std::make_unique<Impl>(result) }
{}

Opm::Action::Result::Result(const Result& rhs)
    : pImpl_ { std::make_unique<Impl>(*rhs.pImpl_) }
{}

Opm::Action::Result::Result(Result&& rhs)
    : pImpl_ { std::move(rhs.pImpl_) }
{}

Opm::Action::Result::~Result() = default;

Opm::Action::Result&
Opm::Action::Result::operator=(const Result& rhs)
{
    this->pImpl_ = std::make_unique<Impl>(*rhs.pImpl_);
    return *this;
}

Opm::Action::Result&
Opm::Action::Result::operator=(Result&& rhs)
{
    this->pImpl_ = std::move(rhs.pImpl_);
    return *this;
}

Opm::Action::Result&
Opm::Action::Result::wells(const std::vector<std::string>& w)
{
    this->pImpl_->matches().addWells(w);
    return *this;
}

bool Opm::Action::Result::conditionSatisfied() const
{
    return this->pImpl_->result();
}

Opm::Action::Result&
Opm::Action::Result::makeSetUnion(const Result& rhs)
{
    this->pImpl_->makeSetUnion(*rhs.pImpl_);
    return *this;
}

Opm::Action::Result&
Opm::Action::Result::makeSetIntersection(const Result& rhs)
{
    this->pImpl_->makeSetIntersection(*rhs.pImpl_);
    return *this;
}

const Opm::Action::Result::MatchingEntities&
Opm::Action::Result::matches() const
{
    return this->pImpl_->matches();
}

bool Opm::Action::Result::operator==(const Result& that) const
{
    return *this->pImpl_ == *that.pImpl_;
}
