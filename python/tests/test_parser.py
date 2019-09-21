import unittest
import os.path
import sys

from opm.io.parser import Parser
from opm.io.parser import ParseContext


class TestParser(unittest.TestCase):

    REGIONDATA = """
START             -- 0
10 MAI 2007 /
RUNSPEC

DIMENS
2 2 1 /
GRID
DX
4*0.25 /
DY
4*0.25 /
DZ
4*0.25 /
TOPS
4*0.25 /
REGIONS
OPERNUM
3 3 1 2 /
FIPNUM
1 1 2 3 /
"""

    MANUALDATA = """
START             -- 0
10 MAI 2007 /
RUNSPEC

DIMENS
2 2 1 /

FIELD
"""

    def setUp(self):
        self.spe3fn = 'tests/spe3/SPE3CASE1.DATA'
        self.norne_fname = os.path.abspath('examples/data/norne/NORNE_ATW2013.DATA')

    def test_create(self):
        parser = Parser()
        deck = parser.parse(self.spe3fn)

        context = ParseContext()
        deck = parser.parse(self.spe3fn, context)

        with open(self.spe3fn) as f:
            string = f.read()
        deck = parser.parse_string(string)
        deck = parser.parse_string(string, context)

    def test_pyinput(self):
        parser = Parser()
        deck = parser.parse_string(self.MANUALDATA)
        with self.assertRaises(ValueError):
            kw = parser["NOT_A_VALID_KEYWORD"]

        kw = parser["FIELD"]
        assert(kw.name == "FIELD")


if __name__ == "__main__":
    unittest.main()

