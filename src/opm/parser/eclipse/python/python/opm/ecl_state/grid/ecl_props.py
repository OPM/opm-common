from cwrap import BaseCClass

from opm import OPMPrototype
from opm.deck import Deck
from opm.ecl_state.grid import EclipseGrid
from opm.ecl_state.tables import TableManager


class Eclipse3DProps(BaseCClass):
    TYPE_NAME = "ecl_props"
    _alloc = OPMPrototype("void* eclipse3d_properties_alloc( deck , table_manager , eclipse_grid)" , bind = False)
    _free  = OPMPrototype("void  eclipse3d_properties_free( ecl_props )")

    
    def __init__(self , deck, tables, grid):
        c_ptr = self._alloc( deck , tables, grid )
        super(Eclipse3DProps , self).__init__( c_ptr )


    def free(self):
        self._free( )
