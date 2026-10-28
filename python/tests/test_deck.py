import os.path
import unittest

from opm.io.parser import Parser

try:
    from tests.utils import test_path
except ImportError:
    from utils import test_path

class TestParse(unittest.TestCase):

    DECK_STRING = """
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
        self.deck = Parser().parse_string(self.DECK_STRING)

    def test_deck_in(self):
        map(lambda kw: self.assertIn(kw, self.deck), [
            'START', 'RUNSPEC', 'DIMENS',
            'GRID', 'TOPS', 'REGIONS',
            'OPERNUM', 'FIPNUM'
        ])
        map(lambda kw: self.assertNotIn(kw, self.deck), [
            'PPCWMAX', 'APIGROUP', 'NOWARN', 'WARN', 'TBLKFAI4', 'WINJMULT'
        ])

    def test_deck_str(self):
        self.assertEqual(
            'DIMENS 2 2 1 /'.split(),
            str(self.deck['DIMENS']).split()
        )
        self.assertEqual(
            'DX 0.25 0.25 0.25 0.25 /'.split(),
            str(self.deck['DX']).split()
        )
        self.assertEqual(
            str(  Parser().parse_string('RUNSPEC\n\nDX\n4*0.5 /')  ).split(),
            'RUNSPEC DX 0.5 0.5 0.5 0.5 /'.split()
        )

    def test_deck_keyword(self):
        for kw in self.deck:
            self.assertTrue(len(kw.name) > 0)
            for rec in kw:
                self.assertTrue( len(rec) > 0 )
                for item in rec:
                    self.assertTrue(len(item) > 0)

    def test_deck_FIPNUM(self):
        self.assertIn('FIPNUM', self.deck)
        self.assertEqual(len(self.deck['FIPNUM']), 1)
        self.assertEqual(len(self.deck['FIPNUM'][0]), 1)
        self.assertEqual(len(self.deck['FIPNUM'][0][0].get_data_list()), 4)


class TestKeywordLocation(unittest.TestCase):
    """Where a keyword was read from.

    SPE1CASE1B.DATA has its PROPS section in the separate file
    props_spe1case1b.inc, pulled in with INCLUDE, so one deck gives both a
    keyword from the top level file and a keyword from an include file.
    """

    def setUp(self):
        self.data_file = test_path("data/SPE1CASE1B.DATA")
        self.deck = Parser().parse(self.data_file)

    def test_keyword_in_top_level_file(self):
        location = self.deck["DIMENS"].location
        self.assertEqual("DIMENS", location.keyword)
        self.assertEqual(self.data_file, location.filename)
        self.assertEqual(24, location.lineno)
        self.assertFalse(location.is_binary())

    def test_keyword_in_include_file(self):
        location = self.deck["PVTW"].location
        self.assertEqual("PVTW", location.keyword)
        self.assertNotEqual(self.data_file, location.filename)
        self.assertTrue(os.path.isabs(location.filename))
        self.assertEqual("props_spe1case1b.inc",
                         os.path.basename(location.filename))
        self.assertEqual(18, location.lineno)

    def test_format(self):
        location = self.deck["DIMENS"].location
        self.assertEqual(
            "DIMENS in {} line 24".format(self.data_file),
            location.format("{keyword} in {file} line {line}")
        )

    def test_repr(self):
        location = self.deck["DIMENS"].location
        self.assertIn("DIMENS", repr(location))
        self.assertIn("24", repr(location))

    def test_location_is_read_only(self):
        location = self.deck["DIMENS"].location
        with self.assertRaises(AttributeError):
            location.lineno = 1

    def test_keyword_parsed_from_string(self):
        deck = Parser().parse_string("RUNSPEC\n\nDIMENS\n2 2 1 /\n")
        location = deck["DIMENS"].location
        self.assertEqual("DIMENS", location.keyword)
        self.assertEqual(3, location.lineno)


if __name__ == "__main__":
    unittest.main()
