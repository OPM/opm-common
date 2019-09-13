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
        self.assertEqual(9, grid.nx)
        self.assertEqual(9, grid.ny)
        self.assertEqual(4, grid.nz)
        self.assertEqual(9*9*4, grid.nactive)
        self.assertEqual(9*9*4, grid.cartesianSize)
        g,i,j,k = 295,7,5,3
        self.assertEqual(g, grid.globalIndex(i,j,k))
        self.assertEqual((i,j,k), grid.getIJK(g))

    def test_simulation(self):
        sim = self.state.simulation()
        self.assertFalse(sim.hasThresholdPressure())
        self.assertFalse(sim.useCPR())
        self.assertTrue(sim.hasDISGAS())
        self.assertTrue(sim.hasVAPOIL())

    def test_tables(self):
        tables = self.state.tables()
        self.assertTrue('SGOF' in tables)
        self.assertTrue('SWOF' in tables)
        self.assertFalse('SOF' in tables)

        ct = self.cp_state.tables()
        self.assertFalse('SGOF' in ct)
        self.assertTrue('SWOF' in ct)

        tab = 'SWOF'
        col = 'KRW'
        self.assertAlmostEqual(0.1345, self.state.tables().evaluate(tab, 0, col, 0.5))
        self.assertAlmostEqual(0.39,   self.state.tables().evaluate(tab, 0, col, 0.72))

        with self.assertRaises(KeyError):
            self.state.tables().evaluate(tab, 0, 'NO', 1)

    def test_faults(self):
        self.assertEquals([], self.state.faultNames())
        parser = Parser()
        faultdeck = parser.parse_string(self.FAULTS_DECK)
        faultstate = EclipseState(faultdeck)
        self.assertEqual(['F1', 'F2'], faultstate.faultNames())
        # 'F2'  5  5  1  4   1  4  'X-' / \n"
        f2 = faultstate.faultFaces('F2')
        self.assertTrue((4,0,0,'X-') in f2)
        self.assertFalse((3,0,0,'X-') in f2)

    def test_jfunc(self):
        # jf["FLAG"]         = WATER; # set in deck
        # jf["DIRECTION"]    = XY;    # default
        # jf["ALPHA_FACTOR"] = 0.5    # default
        # jf["BETA_FACTOR"]  = 0.5    # default
        # jf["OIL_WATER"]    = 21.0   # set in deck
        # jf["GAS_OIL"]      = -1.0   # N/A

        parser = Parser()
        deck = parser.parse('tests/data/JFUNC.DATA')
        js = EclipseState(deck)
        self.assertEqual('JFUNC TEST', js.title)
        jf = js.jfunc()
        print(jf)
        self.assertEqual(jf['FLAG'], 'WATER')
        self.assertEqual(jf['DIRECTION'], 'XY')
        self.assertFalse('GAS_OIL' in jf)
        self.assertTrue('OIL_WATER' in jf)
        self.assertEqual(jf['OIL_WATER'], 21.0)
        self.assertEqual(jf["ALPHA_FACTOR"], 0.5) # default
        self.assertEqual(jf["BETA_FACTOR"],  0.5) # default

        jfunc_gas = """
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
JFUNC
  GAS * 13.0 0.6 0.7 Z /
PROPS\nREGIONS
"""
        deck2 = parser.parse_string(jfunc_gas)
        js_gas = EclipseState(deck2)
        jf = js_gas.jfunc()
        self.assertEqual(jf['FLAG'], 'GAS')
        self.assertEqual(jf['DIRECTION'], 'Z')
        self.assertTrue('GAS_OIL' in jf)
        self.assertFalse('OIL_WATER' in jf)
        self.assertEqual(jf['GAS_OIL'], 13.0)
        self.assertEqual(jf["ALPHA_FACTOR"], 0.6) # default
        self.assertEqual(jf["BETA_FACTOR"],  0.7) # default
        
