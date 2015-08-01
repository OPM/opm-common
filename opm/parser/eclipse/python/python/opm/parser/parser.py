import os.path

from ert.cwrap import BaseCClass
from opm import OPMPrototype
from opm.deck import Deck
from opm.parser import ParseMode


class Parser(BaseCClass):
    TYPE_NAME = "parser"
    _alloc       = OPMPrototype("void* parser_alloc()")
    _free        = OPMPrototype("void  parser_free(parser)")
    _has_keyword = OPMPrototype("bool  parser_has_keyword(parser, char*)")
    _parse_file  = OPMPrototype("deck_obj  parser_parse_file(parser, char*, parse_mode)")
    
    def __init__(self):
        c_ptr = self._alloc()
        super(Parser, self).__init__(c_ptr)

        
    def __contains__(self , kw):
        return self._has_keyword(kw)
        
        
    def free(self):
        self._free( self )

        
    def parseFile(self , filename , parse_mode = None):
        if os.path.isfile( filename ):
            if parse_mode is None:
                parse_mode = ParseMode( )
            return self._parse_file(self , filename, parse_mode)
        else:
            raise IOError("No such file:%s" % filename)

