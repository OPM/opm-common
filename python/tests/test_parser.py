import unittest
import os.path
import sys

import numpy as np

from opm.io.parser import Builtin
from opm.io.parser import Parser
from opm.io.parser import ParseContext
from opm.io.deck import DeckKeyword

from .utils import test_path


unit_foot = 0.3048 #meters

class TestParser(unittest.TestCase):

    REGIONDATA = """
START             -- 0
10 MAI 2007 /
RUNSPEC
FIELD
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
        self.spe3fn = test_path('spe3/SPE3CASE1.DATA')
        self.norne_fname = test_path('../examples/data/norne/NORNE_ATW2013.DATA')

    def test_dynamic_parser(self):
        parser = Parser(add_default = False)
        builtin = Builtin()
        parser.add_keyword( builtin.START )
        parser.add_keyword( builtin.RUNSPEC )
        parser.add_keyword( builtin.FIELD )
        parser.add_keyword( builtin.DIMENS )
        parser.add_keyword( builtin.GRID )
        parser.add_keyword( builtin.DX )
        parser.add_keyword( builtin.DY )
        parser.add_keyword( builtin.DZ )
        parser.add_keyword( builtin.TOPS )
        parser.add_keyword( builtin.REGIONS )
        parser.add_keyword( builtin.OPERNUM )
        parser.add_keyword( builtin.FIPNUM )

        deck = parser.parse_string(self.REGIONDATA)

    def test_dynamic_parser2(self):
        parser = Parser(add_default = False)
        builtin = Builtin()
        kw_list = ["START", "RUNSPEC", "FIELD", "REGIONS", "DIMENS", "GRID", "DX", "DY", "DZ", "TOPS", "OPERNUM", "FIPNUM"]
        for kw in kw_list:
            parser.add_keyword( builtin[kw] )

        deck = parser.parse_string(self.REGIONDATA)

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

    def test_deck_kw_records(self):
        parser = Parser()
        deck = parser.parse_string(self.REGIONDATA)
        active_unit_system = deck.active_unit_system()
        default_unit_system = deck.default_unit_system()
        self.assertEqual(active_unit_system.name, "Field")

        with self.assertRaises(ValueError):
            kw = parser["NOT_A_VALID_KEYWORD"]

        field = parser["FIELD"]
        assert(field.name == "FIELD")

        dkw_field = DeckKeyword(field)
        assert(dkw_field.name == "FIELD")

        DeckKeyword(parser["AQUCWFAC"], [[]], active_unit_system, default_unit_system)

        with self.assertRaises(TypeError):
            dkw_wrong =  DeckKeyword(parser["AQUCWFAC"], [22.2, 0.25], active_unit_system, default_unit_system)

        dkw_aqannc = DeckKeyword(parser["AQANNC"], [[12, 1, 2, 3, 0.89], [13, 4, 5, 6, 0.625]], active_unit_system, default_unit_system)
        assert( len(dkw_aqannc[0]) == 5 )
        assert( dkw_aqannc[0][2].get_int(0) == 2 )
        assert( dkw_aqannc[1][1].get_int(0) == 4 )
        with self.assertRaises(ValueError):
            value = dkw_aqannc[1][1].get_raw(0)
        with self.assertRaises(ValueError):
            value = dkw_aqannc[1][1].get_SI(0)
        assert( dkw_aqannc[1][4].get_raw(0) == 0.625 )
        self.assertAlmostEqual( dkw_aqannc[1][4].get_SI(0), 0.625 * unit_foot**2 )
        assert( dkw_aqannc[1][4].get_raw_data_list() == [0.625] )
        self.assertAlmostEqual( dkw_aqannc[1][4].get_SI_data_list()[0], 0.625 * unit_foot**2 )
        with self.assertRaises(ValueError):
            value = dkw_aqannc[1][4].get_int(0)

        dkw_aqantrc = DeckKeyword(parser["AQANTRC"], [[12, "ABC", 8]], active_unit_system, default_unit_system)
        assert( dkw_aqantrc[0][1].get_str(0) == "ABC" )
        assert( dkw_aqantrc[0][2].get_raw(0) == 8.0 )

        dkw1 = DeckKeyword(parser["AQUCWFAC"], [["*", 0.25]], active_unit_system, default_unit_system)
        assert( dkw1[0][0].get_raw(0) == 0.0 )
        assert( dkw1[0][1].get_raw(0) == 0.25 )

        dkw2 = DeckKeyword(parser["AQUCWFAC"], [[0.25, "*"]], active_unit_system, default_unit_system)
        assert( dkw2[0][0].get_raw(0) == 0.25 )
        assert( dkw2[0][1].get_raw(0) == 1.0 )

        dkw3 = DeckKeyword(parser["AQUCWFAC"], [[0.50]], active_unit_system, default_unit_system)
        assert( dkw3[0][0].get_raw(0) == 0.50 )
        assert( dkw3[0][1].get_raw(0) == 1.0 )

        dkw4 = DeckKeyword(parser["CBMOPTS"], [["3*", "A", "B", "C", "2*", 0.375]], active_unit_system, default_unit_system)
        assert( dkw4[0][0].get_str(0) == "TIMEDEP" )
        assert( dkw4[0][2].get_str(0) == "NOKRMIX" )
        assert( dkw4[0][3].get_str(0) == "A" )
        assert( dkw4[0][6].get_str(0) == "PMPVK" )
        assert( dkw4[0][8].get_raw(0) == 0.375 )
        with self.assertRaises(TypeError):
            dkw4[0][8].get_data_list()

        with self.assertRaises(TypeError):
            DeckKeyword(parser["CBMOPTS"], [["3*", "A", "B", "C", "R2*", 0.77]], active_unit_system, default_unit_system)

        with self.assertRaises(TypeError):
            DeckKeyword(parser["CBMOPTS"], [["3*", "A", "B", "C", "2.2*", 0.77]], active_unit_system, default_unit_system)

        dkw5 = DeckKeyword(parser["AQUCWFAC"], [["2*5.5"]], active_unit_system, default_unit_system)
        assert( dkw5[0][0].get_raw(0) == 5.5 )
        assert( dkw5[0][1].get_raw(0) == 5.5 )

        with self.assertRaises(ValueError):
            raise DeckKeyword(parser["AQANTRC"], [["1*2.2", "ABC", 8]], active_unit_system, default_unit_system)

    def test_deck_kw_records_uda(self):
        parser = Parser()
        deck = parser.parse_string(self.REGIONDATA)
        active_unit_system = deck.active_unit_system()
        default_unit_system = deck.default_unit_system()
        oil_target = 30000  # stb/day
        well_name = "PROD"
        well_status = "OPEN"
        control_mode = "ORAT"
        bhp_limit = 1000  # psia

        wconprod = DeckKeyword(
            parser["WCONPROD"],
            [[well_name, well_status, control_mode, oil_target, "4*", bhp_limit]],
            active_unit_system, default_unit_system)

        self.assertEqual(len(wconprod), 1)
        record = wconprod[0]
        self.assertEqual(record[0].name(), "WELL")
        self.assertTrue(record[0].is_string())
        self.assertEqual(record[0].get_str(0), "PROD")
        self.assertEqual(record[1].name(), "STATUS")
        self.assertTrue(record[1].is_string())
        self.assertEqual(record[1].get_str(0), "OPEN")
        self.assertEqual(record[2].name(), "CMODE")
        self.assertTrue(record[2].is_string())
        self.assertEqual(record[2].get_str(0), "ORAT")
        self.assertEqual(record[3].name(), "ORAT")
        self.assertTrue(record[3].is_uda())
        self.assertEqual(record[3].value.value, 30000)
        self.assertEqual(record[4].name(), "WRAT")
        self.assertTrue(record[4].is_uda())
        self.assertEqual(record[4].value.value, 0)
        self.assertEqual(record[5].name(), "GRAT")
        self.assertTrue(record[5].is_uda())
        self.assertEqual(record[5].value.value, 0)
        self.assertEqual(record[6].name(), "LRAT")
        self.assertTrue(record[6].is_uda())
        self.assertEqual(record[6].value.value, 0)
        self.assertEqual(record[7].name(), "RESV")
        self.assertTrue(record[7].is_uda())
        self.assertEqual(record[7].value.value, 0)
        self.assertEqual(record[8].name(), "BHP")
        self.assertTrue(record[8].is_uda())
        self.assertEqual(record[8].value.value, 1000)

    def test_deck_kw_vector(self):
        parser = Parser()
        deck = parser.parse_string(self.REGIONDATA)
        active_unit_system = deck.active_unit_system()
        default_unit_system = deck.default_unit_system()
        self.assertEqual(active_unit_system.name, "Field")

        int_array = np.array([0, 1, 2, 3])
        hbnum_kw = DeckKeyword( parser["HBNUM"], int_array)
        assert( np.array_equal(hbnum_kw.get_int_array(), int_array) )

        raw_array = np.array([1.1, 2.2, 3.3])
        zcorn_kw = DeckKeyword( parser["ZCORN"], raw_array, active_unit_system, default_unit_system)
        assert( np.array_equal(zcorn_kw.get_raw_array(), raw_array) )
        si_array = zcorn_kw.get_SI_array()
        self.assertAlmostEqual( si_array[0], 1.1 * unit_foot )
        self.assertAlmostEqual( si_array[2], 3.3 * unit_foot )

        assert( not( "ZCORN" in deck ) )
        deck.add( zcorn_kw )
        assert( "ZCORN" in deck )



if __name__ == "__main__":
    unittest.main()
