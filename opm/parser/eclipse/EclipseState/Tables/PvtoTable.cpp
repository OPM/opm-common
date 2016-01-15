#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/ColumnSchema.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvtoTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableSchema.hpp>

namespace Opm {
    PvtoTable::PvtoTable(Opm::DeckKeywordConstPtr keyword , size_t tableIdx) : PvtxTable("RS") {
        m_underSaturatedSchema = std::make_shared<TableSchema>( );

        m_underSaturatedSchema->addColumn( ColumnSchema( "P"  , Table::STRICTLY_INCREASING , Table::DEFAULT_NONE ));
        m_underSaturatedSchema->addColumn( ColumnSchema( "BO" , Table::RANDOM , Table::DEFAULT_LINEAR ));
        m_underSaturatedSchema->addColumn( ColumnSchema( "MU" , Table::RANDOM , Table::DEFAULT_LINEAR ));



        m_saturatedSchema = std::make_shared<TableSchema>( );
        m_saturatedSchema->addColumn( ColumnSchema( "RS" , Table::STRICTLY_INCREASING , Table::DEFAULT_NONE ));
        m_saturatedSchema->addColumn( ColumnSchema( "P"  , Table::RANDOM , Table::DEFAULT_NONE ));
        m_saturatedSchema->addColumn( ColumnSchema( "BO" , Table::RANDOM , Table::DEFAULT_LINEAR ));
        m_saturatedSchema->addColumn( ColumnSchema( "MU" , Table::RANDOM , Table::DEFAULT_LINEAR ));

        PvtxTable::init(keyword , tableIdx);
    }
}
