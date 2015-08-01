import os.path

from ert.cwrap import BaseCClass
from opm import OPMPrototype


class TableManager(BaseCClass):
    TYPE_NAME = "table_manager"
    _alloc    = OPMPrototype("void* table_manager_alloc(deck)")
    _free     = OPMPrototype("void* table_manager_free(table_manager)")
    
    def __init__(self , deck):
        c_ptr = self._alloc(deck)
        super(TableManager, self).__init__(c_ptr)


    def free(self):
        self._free( self )

        
