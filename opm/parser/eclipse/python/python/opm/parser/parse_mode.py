from ert.cwrap import BaseCClass, CWrapper
from opm import OPMPrototype

class ParseMode(BaseCClass):
    TYPE_NAME = "parse_mode"
    _alloc    = OPMPrototype("void* parse_mode_alloc()")
    _free     = OPMPrototype("void  parse_mode_free(parse_mode)")
    _update   = OPMPrototype("void  parse_mode_update(parse_mode, char* , error_action_enum)")
    
    def __init__(self):
        c_ptr = self._alloc()
        super(ParseMode, self).__init__(c_ptr)

        
    def free(self):
        self._free( self )


    def update(self , var , action):
        self._update( self , var , action )

