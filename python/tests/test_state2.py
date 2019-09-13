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
        
        #cls.spe3 = opm.io.parse('tests/spe3/SPE3CASE1.DATA')
        #cpa = opm.io.parse('tests/data/CORNERPOINT_ACTNUM.DATA')
        #cls.state = cls.spe3.state
        #cls.cp_state = cpa.state
        pass
        #parser = Parser()
        #cls.deck_spe3 = parser.parse_string('tests/spe3/SPE3CASE1.DATA')
        #cls.deck_cpa  = parser.parse_string('tests/data/CORNERPOINT_ACTNUM.DATA')
        #cls.state    = EclipseState(cls.deck_spe3)
        #cls.cp_state = EclipseState(cls.deck_cpa)
        
