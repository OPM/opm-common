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

#ifndef OPM_TABLE_CONTAINER_HPP
#define OPM_TABLE_CONTAINER_HPP

#include <cstddef>
#include <map>
#include <memory>

namespace Opm {

    class SimpleTable;

    /// The TableContainer class implements a simple map:
    ///
    ///    {tableNumber, Table}
    ///
    /// The main functionality of the TableContainer class is that the
    /// getTable() method implements the ECLIPSE behavior:
    ///
    /// If table N is not implemented - use table N - 1.
    ///
    /// The getTable() method will eventually throw an exception if not even
    /// table 0 is there.
    ///
    /// Consider the following code:
    ///
    /// TableContainer container(10);
    ///
    /// std::shared_ptr<TableType> table0 = std::make_shared<TableType>(...);
    /// container.addTable(table0, 0)
    ///
    /// We create a container with maximum 10 tables, and then we add one
    /// single table at slot 0; then we have:
    ///
    /// container.size()         == 1
    /// container.hasTable( 0 )  == true
    /// container.hasTable( 9 )  == false
    /// container.hasTable(10 )  == false
    ///
    /// container.getTable( 0 ) == container[9] == table0;
    /// container.gteTable(10 ) ==> exception
    class TableContainer
    {
    public:
        using TableMap = std::map<size_t, std::shared_ptr<SimpleTable>>;

        TableContainer();
        explicit TableContainer(size_t maxTables);

        static TableContainer serializationTestObject();

        bool empty() const;

        // This is the number of actual tables in the container.
        size_t size() const;
        size_t max() const;

        const TableMap& tables() const;

        void addTable(size_t tableNumber, std::shared_ptr<SimpleTable> table);

        // Observe that the hasTable() method does not invoke the "If table
        // N is not implemented use table N - 1 behavior.
        bool hasTable(size_t tableNumber) const;
        const SimpleTable& getTable(size_t tableNumber) const;

        const SimpleTable& operator[](size_t tableNumber) const
        {
            return this->getTable(tableNumber);
        }

        template <class TableType>
        const TableType& getTable(size_t tableNumber) const
        {
            // This is, strictly speaking, a downcast so we should prefer
            // dynamic_cast<>() instead.  However, serializeOp() by
            // construction throws away the derived TableType during object
            // distribution, keeping only the SimpleTable, so dynamic_cast<>
            // will throw a bad_cast exception on ranks other than the I/O
            // rank (0).  We therefore resort to static_cast<>() here
            // instead and hope that the caller specifies the correct
            // derived type...
            return static_cast<const TableType&>(this->getTable(tableNumber));
        }

        bool operator==(const TableContainer& data) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_maxTables);
            serializer(m_tables);
        }

    private:
        size_t m_maxTables;
        TableMap m_tables;
    };

}

#endif // OPM_TABLE_CONTAINER_HPP
