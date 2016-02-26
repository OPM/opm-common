#include <opm/parser/eclipse/Utility/Functional.hpp>

namespace Opm {
namespace fun {

    iota::iota( int begin, int end ) : first( begin ), last( end ) {}

    iota::iota( int end ) : iota( 0, end ) {}

    size_t iota::size() const {
        return this->last - this->first;
    }

    iota::const_iterator iota::begin() const {
        return { first };
    }

    iota::const_iterator iota::end() const {
        return { last };
    }

    int iota::const_iterator::operator*() const {
        return this->value;
    }

    iota::const_iterator& iota::const_iterator::operator++() {
        ++( this->value );
        return *this;
    }

    iota::const_iterator iota::const_iterator::operator++( int ) {
        iota::const_iterator copy( *this );
        this->operator++();
        return copy;
    }

    bool iota::const_iterator::operator==( const const_iterator& rhs ) const {
        return this->value == rhs.value;
    }

    bool iota::const_iterator::operator!=( const const_iterator& rhs ) const {
        return !(*this == rhs );
    }

    iota::const_iterator::const_iterator( int x ) : value( x ) {}


}
}
