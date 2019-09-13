import unittest
import opm.io

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
    """
    @classmethod
    def setUpClass(cls):
        cls.spe3 = opm.io.parse('tests/spe3/SPE3CASE1.DATA')
        cpa = opm.io.parse('tests/data/CORNERPOINT_ACTNUM.DATA')
        cls.state = cls.spe3.state
        cls.cp_state = cpa.state
    

    def test_summary(self):
        smry = self.spe3.summary_config
        self.assertTrue('SummaryConfig' in repr(smry))
        self.assertTrue('WOPR' in smry) # hasKeyword
        self.assertFalse('NONO' in smry) # hasKeyword
    

    def test_jfunc(self):
        # jf["FLAG"]         = WATER; # set in deck
        # jf["DIRECTION"]    = XY;    # default
        # jf["ALPHA_FACTOR"] = 0.5    # default
        # jf["BETA_FACTOR"]  = 0.5    # default
        # jf["OIL_WATER"]    = 21.0   # set in deck
        # jf["GAS_OIL"]      = -1.0   # N/A

        js = opm.io.parse('tests/data/JFUNC.DATA').state
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

        jfunc_gas = ###RUNSPEC
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
###
        js_gas = opm.io.parse_string(jfunc_gas).state
        jf = js_gas.jfunc()
        self.assertEqual(jf['FLAG'], 'GAS')
        self.assertEqual(jf['DIRECTION'], 'Z')
        self.assertTrue('GAS_OIL' in jf)
        self.assertFalse('OIL_WATER' in jf)
        self.assertEqual(jf['GAS_OIL'], 13.0)
        self.assertEqual(jf["ALPHA_FACTOR"], 0.6) # default
        self.assertEqual(jf["BETA_FACTOR"],  0.7) # default
    """

if __name__ == "__main__":
    unittest.main()
