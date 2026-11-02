import unittest

from opm.io.parser import Parser, eclSectionType
from opm.io.deck import DeckKeyword

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

    def test_add_unit_keyword_with_section(self):
        parser = Parser()
        deck = parser.parse_string("RUNSPEC\n")
        deck.add(DeckKeyword(parser["FIELD"]), eclSectionType.RUNSPEC)
        self.assertEqual(deck.active_unit_system().name, "Field")

        # The supplied section applies even without a stored section header.
        deck.add(DeckKeyword(parser["METRIC"]), eclSectionType.PROPS)
        self.assertNotIn("PROPS", deck)
        self.assertEqual(deck.active_unit_system().name, "Field")

        deck.add(DeckKeyword(parser["GRID"]), eclSectionType.GRID)
        deck.add(DeckKeyword(parser["METRIC"]), eclSectionType.GRID)
        self.assertEqual(deck.active_unit_system().name, "Field")
        self.assertEqual(deck.count("METRIC"), 2)

    def test_add_requires_section(self):
        parser = Parser()
        deck = parser.parse_string("RUNSPEC\n")
        keyword = DeckKeyword(parser["FIELD"])
        with self.assertRaises(TypeError):
            deck.add(keyword)
        with self.assertRaises(TypeError):
            deck.add(keyword, None)
        self.assertNotIn("FIELD", deck)

    def test_deck_FIPNUM(self):
        self.assertIn('FIPNUM', self.deck)
        self.assertEqual(len(self.deck['FIPNUM']), 1)
        self.assertEqual(len(self.deck['FIPNUM'][0]), 1)
        self.assertEqual(len(self.deck['FIPNUM'][0][0].get_data_list()), 4)


if __name__ == "__main__":
    unittest.main()
