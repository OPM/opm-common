/*
  Copyright 2013 Statoil ASA.

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

#ifndef GROUPTREE_HPP
#define GROUPTREE_HPP

#include <string>
#include <vector>
#include <map>

namespace Opm {

class GroupTree {
public:
    struct group {
        std::string name;
        std::string parent;

        bool operator<( const group& rhs ) const;
        bool operator==( const std::string& name ) const;
        bool operator!=( const std::string& name ) const;

        bool operator<( const std::string& name ) const;
        bool operator==( const group& rhs ) const;
        bool operator!=( const group& rhs ) const;
    };

    void update( const std::string& name);
    void update( const std::string& name, const std::string& parent);
    void updateSeqIndex( const std::string& name, const std::string& other_parent);
    bool exists( const std::string& group ) const;
    const std::string& parent( const std::string& name ) const;
    std::vector< std::string > children( const std::string& parent ) const;
    const std::map<std::string , size_t>& nameSeqIndMap() const;
    const std::map<size_t, std::string >& seqIndNameMap() const;
    bool operator==( const GroupTree& ) const;
    bool operator!=( const GroupTree& ) const;
    std::vector<GroupTree::group>::const_iterator begin() const;
    std::vector<GroupTree::group>::const_iterator end() const;

private:
    std::vector< group >::iterator find( const std::string& );
    friend bool operator<( const std::string&, const group& );

    std::vector< group > groups = { group { "FIELD", "" } };
    std::map<std::string , size_t> m_nameSeqIndMap;
    std::map<size_t, std::string > m_seqIndNameMap;
};


std::ostream& operator<<(std::ostream& stream, const GroupTree& gt);


}

#endif /* GROUPTREE_HPP */

