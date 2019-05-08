import unittest
import sunbeam

class TestState(unittest.TestCase):
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
        cls.spe3 = sunbeam.parse('spe3/SPE3CASE1.DATA')
        cpa = sunbeam.parse('data/CORNERPOINT_ACTNUM.DATA')
        cls.state = cls.spe3.state
        cls.cp_state = cpa.state

    def test_repr_title(self):
        self.assertTrue('EclipseState' in repr(self.state))
        self.assertEqual('SPE 3 - CASE 1', self.state.title)

    def test_state_nnc(self):
        self.assertFalse(self.state.has_input_nnc())

    def test_grid(self):
        grid = self.state.grid()
        self.assertTrue('EclipseGrid' in repr(grid))
        self.assertEqual(9, grid.getNX())
        self.assertEqual(9, grid.getNY())
        self.assertEqual(4, grid.getNZ())
        self.assertEqual(9*9*4, grid.nactive())
        self.assertEqual(9*9*4, grid.cartesianSize())
        g,i,j,k = 295,7,5,3
        self.assertEqual(g, grid.globalIndex(i,j,k))
        self.assertEqual((i,j,k), grid.getIJK(g))

    def test_config(self):
        cfg = self.state.cfg()
        self.assertTrue('EclipseConfig' in repr(cfg))

        init = cfg.init()
        self.assertTrue(init.hasEquil())
        self.assertFalse(init.restartRequested())
        self.assertEqual(0, init.getRestartStep())

        rst = cfg.restart()
        self.assertFalse(rst.getWriteRestartFile(0))
        self.assertEqual(7, rst.getFirstRestartStep())

    def test_summary(self):
        smry = self.spe3.summary_config
        self.assertTrue('SummaryConfig' in repr(smry))
        self.assertTrue('WOPR' in smry) # hasKeyword
        self.assertFalse('NONO' in smry) # hasKeyword

    def test_simulation(self):
        sim = self.state.simulation()
        self.assertFalse(sim.hasThresholdPressure())
        self.assertFalse(sim.useCPR())
        self.assertTrue(sim.hasDISGAS())
        self.assertTrue(sim.hasVAPOIL())

    def test_tables(self):
        tables = self.state.table
        self.assertTrue('SGOF' in tables)
        self.assertTrue('SWOF' in tables)
        self.assertFalse('SOF' in tables)

        ct = self.cp_state.table
        self.assertFalse('SGOF' in ct)
        self.assertTrue('SWOF' in ct)

        tab = 'SWOF'
        col = 'KRW'
        self.assertAlmostEqual(0.1345, self.state.table[tab](col, 0.5))
        self.assertAlmostEqual(0.39,   self.state.table[tab](col, 0.72))

        self.assertAlmostEqual(0.1345, self.state.table[tab, col](0.5))
        self.assertAlmostEqual(0.39,   self.state.table[tab, col](0.72))

        with self.assertRaises(KeyError):
            self.state.table[tab, 'NO'](1)


    def test_faults(self):
        self.assertEquals([], self.state.faultNames())
        self.assertEquals({}, self.state.faults())
        faultdeck = sunbeam.parse_string(self.FAULTS_DECK).state
        self.assertEqual(['F1', 'F2'], faultdeck.faultNames())
        # 'F2'  5  5  1  4   1  4  'X-' / \n"
        f2 = faultdeck.faultFaces('F2')
        self.assertTrue((4,0,0,'X-') in f2)
        self.assertFalse((3,0,0,'X-') in f2)

    def test_jfunc(self):
        # jf["FLAG"]         = WATER; # set in deck
        # jf["DIRECTION"]    = XY;    # default
        # jf["ALPHA_FACTOR"] = 0.5    # default
        # jf["BETA_FACTOR"]  = 0.5    # default
        # jf["OIL_WATER"]    = 21.0   # set in deck
        # jf["GAS_OIL"]      = -1.0   # N/A

        js = sunbeam.parse('data/JFUNC.DATA').state
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

        jfunc_gas = """RUNSPEC
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
        js_gas = sunbeam.parse_string(jfunc_gas).state
        jf = js_gas.jfunc()
        self.assertEqual(jf['FLAG'], 'GAS')
        self.assertEqual(jf['DIRECTION'], 'Z')
        self.assertTrue('GAS_OIL' in jf)
        self.assertFalse('OIL_WATER' in jf)
        self.assertEqual(jf['GAS_OIL'], 13.0)
        self.assertEqual(jf["ALPHA_FACTOR"], 0.6) # default
        self.assertEqual(jf["BETA_FACTOR"],  0.7) # default

if __name__ == "__main__":
    unittest.main()
