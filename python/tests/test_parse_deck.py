import unittest
import json
import opm
import opm.io
import os.path
import numpy as np

try:
    from tests.utils import test_path
except ImportError:
    from utils import test_path

from opm.io.parser import Parser, ParseContext, eclSectionType
from opm.io.deck import DeckKeyword


class TestParse(unittest.TestCase):

    DECK_ADDITIONAL_KEYWORDS = """
START             -- 0
10 MAI 2007 /
RUNSPEC
TESTKEY0
1 2 3 4 5 6 7 8 /
TESTKEY1
TESTKEY2
    /
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

    KEYWORDS = [{
        "name" : "TESTKEY0",
        "sections" : [ "RUNSPEC" ],
        "data" : {"value_type" : "INT" }
    }, {
        "name" : "TESTKEY1",
        "sections" : [ "RUNSPEC" ],
        "size":0
    }, {
        "name" : "TESTKEY2",
        "sections" : [ "RUNSPEC" ],
        "size":1,
        "items":[
            { "name":"TEST", "value_type":"STRING", "default":"DEFAULT" }
        ]
    }, ]

    def setUp(self):
        self.spe3fn = 'spe3/SPE3CASE1.DATA'
        self.norne_fname = os.path.abspath('../../examples/data/norne/NORNE_ATW2013.DATA')

    def test_IOError(self):
        with self.assertRaises(Exception):
            Parser().parse("file/not/found")


    def test_parser_fail_without_extension(self):
        error_recovery = [("PARSE_RANDOM_SLASH", opm.io.action.ignore)]
        with self.assertRaises(RuntimeError):
            parse_context = ParseContext(error_recovery)
            deck = Parser().parse_string(self.DECK_ADDITIONAL_KEYWORDS, parse_context)


    def test_parser_extension(self):
        error_recovery = [("PARSE_RANDOM_SLASH", opm.io.action.ignore)]

        parse_context = ParseContext(error_recovery)
        parser = Parser()
        for kw in self.KEYWORDS:
            parser.add_keyword(json.dumps(kw))

        deck = parser.parse_string(self.DECK_ADDITIONAL_KEYWORDS, parse_context)

        self.assertIn( 'TESTKEY0', deck )
        self.assertIn( 'TESTKEY1', deck )
        self.assertIn( 'TESTKEY2', deck )


    def test_parser_deckItems(self):

        parser = Parser()

        error_recovery = [("PARSE_RANDOM_SLASH", opm.io.action.ignore),
                          ("PARSE_EXTRA_RECORDS", opm.io.action.ignore)]

        context = ParseContext(error_recovery)

        self.deck_spe1case1 = parser.parse(test_path("data/SPE1CASE1.DATA"), context)

        dkw_compdate = self.deck_spe1case1["COMPDAT"]

        self.assertTrue( dkw_compdate[0][0].is_string() )
        self.assertFalse( dkw_compdate[0][1].is_string() )

        self.assertTrue( dkw_compdate[0][1].is_int() )
        self.assertFalse( dkw_compdate[0][1].is_double() )

        self.assertTrue( dkw_compdate[0][8].is_double() )

        self.assertTrue(dkw_compdate[0][0].value == "PROD")

        conI = dkw_compdate[0][1].value
        conJ = dkw_compdate[0][2].value
        conK = dkw_compdate[0][3].value

        self.assertEqual(dkw_compdate[0][5].value, "OPEN")

        self.assertTrue((conI, conJ, conK) == (10,10,3))

        self.assertFalse( dkw_compdate[0][7].valid )
        self.assertTrue( dkw_compdate[0][7].defaulted )

        self.assertEqual( dkw_compdate[0][6].value, 0)

        self.assertEqual( dkw_compdate[0][8].value, 0.5)

        dkw_wconprod = self.deck_spe1case1["WCONPROD"]

        welln= dkw_wconprod[0][0].value
        self.assertEqual(dkw_wconprod[0][2].value, "ORAT")
        self.assertEqual(dkw_wconprod[0][3].value, "WUOPRL")
        self.assertEqual(dkw_wconprod[0][5].value, 1.5e5)

        self.assertTrue(not dkw_wconprod[0][3].defaulted)
        self.assertTrue(dkw_wconprod[0][3].is_uda)

        dkw_wconinje = self.deck_spe1case1["WCONINJE"]

        self.assertEqual(dkw_wconinje[0][0].value, "INJ")
        self.assertTrue(dkw_wconinje[0][4].is_uda)
        self.assertEqual(dkw_wconinje[0][4].value, 1e5)

        dkw_permx = self.deck_spe1case1["PERMX"]
        permx =  dkw_permx.get_raw_array()
        self.assertEqual(len(permx), 300)
        self.assertTrue(isinstance(permx, np.ndarray))
        self.assertEqual(permx.dtype, "float64")

        dkw_eqlnum = self.deck_spe1case1["EQLNUM"]
        eqlnum =  dkw_eqlnum.get_int_array()

        self.assertEqual(len(eqlnum), 300)
        self.assertTrue(isinstance(eqlnum, np.ndarray))
        self.assertEqual(eqlnum.dtype, "int32")


    def test_parser_section_deckItems(self):

        all_spe1case1 = ["RUNSPEC", "TITLE", "DIMENS", "EQLDIMS", "TABDIMS", "REGDIMS", "OIL", "GAS",
                         "WATER", "DISGAS", "FIELD", "START", "WELLDIMS", "UNIFOUT", "UDQDIMS", "UDADIMS",
                         "GRID", "INIT", "NOECHO", "DX", "DY", "DZ", "TOPS", "PORO", "PERMX", "PERMY",
                         "PERMZ", "ECHO", "PROPS", "PVTW", "ROCK", "SWOF", "SGOF", "DENSITY", "PVDG",
                         "PVTO", "REGIONS", "EQLNUM", "FIPNUM", "SOLUTION", "EQUIL", "RSVD", "SUMMARY",
                         "FOPR", "WGOR", "FGOR", "BPR", "BGSAT", "WBHP", "WGIR", "WGIT", "WGPR", "WGPT",
                         "WOIR", "WOIT", "WOPR", "WOPT", "WWIR", "WWIT", "WWPR", "WWPT", "WUOPRL", "SCHEDULE",
                         "UDQ", "RPTSCHED", "RPTRST", "DRSDT", "WELSPECS", "COMPDAT", "WCONPROD", "WCONINJE", "TSTEP"]

        # notice that RUNSPEC keywords will always be parsed since these properties from these keyword
        # are needed to parse following sections.

        props_spe1case1 = ["RUNSPEC", "TITLE", "DIMENS", "EQLDIMS", "TABDIMS", "REGDIMS", "OIL", "GAS",
                         "WATER", "DISGAS", "FIELD", "START", "WELLDIMS", "UNIFOUT", "UDQDIMS", "UDADIMS", "GRID",
                          "PVTW", "ROCK", "SWOF", "SGOF", "DENSITY", "PVDG",
                         "PVTO"]

        parser = Parser()

        error_recovery = [("PARSE_RANDOM_SLASH", opm.io.action.ignore),
                          ("PARSE_EXTRA_RECORDS", opm.io.action.ignore)]

        context = ParseContext(error_recovery)

        deck1 = parser.parse(test_path("data/SPE1CASE1.DATA"), context)

        self.assertEqual( len(deck1), len(all_spe1case1))

        test_1 = [dkw.name for dkw in deck1]

        for test, ref in zip(test_1, all_spe1case1):
            self.assertEqual( test, ref)

        section_list = [eclSectionType.PROPS]

        deck2 = parser.parse(test_path("data/SPE1CASE1.DATA"), context, section_list)

        self.assertEqual( len(deck2), len(props_spe1case1))

        test_2 = [dkw.name for dkw in deck2]

        for test, ref in zip(test_2, props_spe1case1):
            self.assertEqual( test, ref)

        # props section keyword located in include file for this deck (SPE1CASE1B.DATA)
        # not possible to parse individual sections

        with self.assertRaises(RuntimeError):
            parser.parse(test_path("data/SPE1CASE1B.DATA"), context, section_list)



if __name__ == "__main__":
    unittest.main()
