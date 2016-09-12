from unittest import TestCase
import os.path
from ert.test import TestAreaContext

from opm.parser import Parser
from opm.ecl_state.grid import EclipseGrid



class EclipseGridTest(TestCase):
    def setUp(self):
        pass

    def test_parse(self):
        p = Parser()
        test_file = os.path.join( os.path.abspath( os.path.dirname(__file__ )) , "../../../../../../../testdata/integration_tests/GRID/CORNERPOINT_ACTNUM.DATA")
        deck = p.parseFile( test_file )
        grid = EclipseGrid( deck )
        
