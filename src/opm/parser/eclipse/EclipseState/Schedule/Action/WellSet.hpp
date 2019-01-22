
/*
  Small utility class to hold information about which wells match the
  various expressions in the ACTIONX parsing.
*/
#ifndef ACTION_WELLSET
#define ACTION_WELLSET

#include <unordered_set>
#include <vector>

namespace Opm {
class WellSet {
public:
    void add(const std::string& well) { this->well_set.insert(well); }
    std::size_t size() const { return this->well_set.size(); }

    std::vector<std::string> wells() const {
        return std::vector<std::string>( this->well_set.begin(), this->well_set.end() );
    }

    WellSet& intersect(const WellSet& other) {
        for (auto& well : this->well_set)
            if (!other.contains(well))
                this->well_set.erase(well);
        return *this;
    }

    WellSet& add(const WellSet& other) {
        for (auto& well : other.well_set)
            this->add(well);
        return *this;
    }

    bool contains(const std::string& well) const {
        return (this->well_set.find(well) != this->well_set.end());
    }

private:
    std::unordered_set<std::string> well_set;
};

}

#endif
