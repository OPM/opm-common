#include <opm/parser/eclipse/Parser/ParseMode.hpp>
#include <opm/parser/eclipse/Parser/InputErrorAction.hpp>

extern "C" {


    Opm::ParseMode * parse_mode_alloc() {
        Opm::ParseMode * parse_mode = new Opm::ParseMode( );
        return parse_mode;
    }


    void parse_mode_free( Opm::ParseMode * parse_mode ) {
        delete parse_mode;
    }


    void parse_mode_update( Opm::ParseMode * parse_mode , const char * var , Opm::InputError::Action action) {
        parse_mode->update( var , action );
    }

}
