import unittest

from opm.io.parser import Parser

from opm.io.ecl_state import EclipseConfig, EclipseGrid, Eclipse3DProperties, Tables, EclipseState


class TestState2(unittest.TestCase):
    FAULTS_DECK = """
RUNSPEC

DIMENS
 10 10 10 /
GRID
DX
1000*0.25 /
DY
1000*0.25 /
DZ
1000*0.25 /
TOPS
100*0.25 /
FAULTS
  'F1'  1  1  1  4   1  4  'X' /
  'F2'  5  5  1  4   1  4  'X-' /
/
MULTFLT
  'F1' 0.50 /
  'F2' 0.50 /
/
EDIT
MULTFLT /
  'F2' 0.25 /
/
OIL

GAS

TITLE
The title

START
8 MAR 1998 /

PROPS
REGIONS
SWAT
1000*1 /
SATNUM
1000*2 /
\
"""

    @classmethod
    def setUpClass(cls):      
        parser = Parser()
        cls.deck_spe3 = parser.parse('tests/spe3/SPE3CASE1.DATA')
        cls.deck_cpa  = parser.parse('tests/data/CORNERPOINT_ACTNUM.DATA')
        cls.state    = EclipseState(cls.deck_spe3)
        cls.cp_state = EclipseState(cls.deck_cpa)

    def test_repr_title(self):
        self.assertEqual('SPE 3 - CASE 1', self.state.title)

    def test_state_nnc(self):
        self.assertFalse(self.state.has_input_nnc())

    def test_grid(self):
        grid = self.state.grid()
        self.assertEqual(9, grid.NX)
        self.assertEqual(9, grid.NY)
        self.assertEqual(4, grid.NZ)
        self.assertEqual(9*9*4, grid.nactive)
        self.assertEqual(9*9*4, grid.cartesianSize)
        g,i,j,k = 295,7,5,3
        self.assertEqual(g, grid.globalIndex(i,j,k))
        self.assertEqual((i,j,k), grid.getIJK(g))


        
