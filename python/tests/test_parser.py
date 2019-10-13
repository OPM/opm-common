import unittest
import os.path
import sys

from opm.io.parser import Parser
from opm.io.parser import ParseContext

from opm.io.deck import DeckKeyword


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


    def setUp(self):
        self.spe3fn = 'tests/spe3/SPE3CASE1.DATA'
        self.norne_fname = os.path.abspath('examples/data/norne/NORNE_ATW2013.DATA')

    def test_create(self):
        parser = Parser()
        deck = parser.parse(self.spe3fn)
        active_unit_system = deck.active_unit_system()
        default_unit_system = deck.default_unit_system()
        self.assertEqual(active_unit_system.name, "Field")

        context = ParseContext()
        deck = parser.parse(self.spe3fn, context)

        with open(self.spe3fn) as f:
            string = f.read()
        deck = parser.parse_string(string)
        deck = parser.parse_string(string, context)

    def test_create_deck_kw(self):
        parser = Parser()
        with self.assertRaises(ValueError):
            kw = parser["NOT_A_VALID_KEYWORD"]

        field = parser["FIELD"]
        assert(field.name == "FIELD")

        dkw_field = DeckKeyword(field)
        assert(dkw_field.name == "FIELD")

        DeckKeyword(parser["AQUCWFAC"], [[]])

        with self.assertRaises(TypeError):
            dkw_wrong =  DeckKeyword(parser["AQUCWFAC"], [22.2, 0.25])

        dkw_aqannc = DeckKeyword(parser["AQANNC"], [[12, 1, 2, 3, 0.89], [13, 4, 5, 6, 0.625]])
        assert( len(dkw_aqannc[0]) == 5 )
        assert( dkw_aqannc[0][2][0] == 2 )
        assert( dkw_aqannc[1][1][0] == 4 )
        assert( dkw_aqannc[1][4][0] == 0.625 )

        dkw_aqantrc = DeckKeyword(parser["AQANTRC"], [[12, "ABC", 8]])
        assert( dkw_aqantrc[0][1][0] == "ABC" )
        assert( dkw_aqantrc[0][2][0] == 8.0 )

        dkw1 = DeckKeyword(parser["AQUCWFAC"], [["*", 0.25]])
        assert( dkw1[0][0][0] == 0.0 )
        assert( dkw1[0][1][0] == 0.25 )

        dkw2 = DeckKeyword(parser["AQUCWFAC"], [[0.25, "*"]])
        assert( dkw2[0][0][0] == 0.25 )
        assert( dkw2[0][1][0] == 1.0 )

        dkw3 = DeckKeyword(parser["AQUCWFAC"], [[0.50]])
        assert( dkw3[0][0][0] == 0.50 )
        assert( dkw3[0][1][0] == 1.0 )

        dkw4 = DeckKeyword(parser["CBMOPTS"], [["3*", "A", "B", "C", "2*", 0.375]])
        assert( dkw4[0][0][0] == "TIMEDEP" )
        assert( dkw4[0][2][0] == "NOKRMIX" )
        assert( dkw4[0][3][0] == "A" )
        assert( dkw4[0][6][0] == "PMPVK" )
        assert( dkw4[0][8][0] == 0.375 )

        with self.assertRaises(TypeError):
            DeckKeyword(parser["CBMOPTS"], [["3*", "A", "B", "C", "R2*", 0.77]])

        with self.assertRaises(TypeError):
            DeckKeyword(parser["CBMOPTS"], [["3*", "A", "B", "C", "2.2*", 0.77]])

        dkw5 = DeckKeyword(parser["AQUCWFAC"], [["2*5.5"]])
        assert( dkw5[0][0][0] == 5.5 )
        assert( dkw5[0][1][0] == 5.5 )

        with self.assertRaises(ValueError):
            raise DeckKeyword(parser["AQANTRC"], [["1*2.2", "ABC", 8]])
        


if __name__ == "__main__":
    unittest.main()

