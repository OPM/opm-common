import unittest
import sunbeam

class TestParse(unittest.TestCase):

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
        self.spe3fn = 'spe3/SPE3CASE1.DATA'

    def testParse(self):
        spe3 = sunbeam.parse(self.spe3fn)
        self.assertEqual('SPE 3 - CASE 1', spe3.title)

    def testParseWithAction(self):
        action = ("PARSE_RANDOM_SLASH", sunbeam.action.ignore)
        spe3 = sunbeam.parse(self.spe3fn, action)
        self.assertEqual('SPE 3 - CASE 1', spe3.title)

    def testParseWithMultipleActions(self):
        actions = [ ("PARSE_RANDOM_SLASH", sunbeam.action.ignore),
                    ("FOO", sunbeam.action.warn),
                    ("PARSE_RANDOM_TEXT", sunbeam.action.throw) ]

        spe3 = sunbeam.parse(self.spe3fn, actions)
        self.assertEqual('SPE 3 - CASE 1', spe3.title)

    def testThrowOnInvalidAction(self):
        actions = [ ("PARSE_RANDOM_SLASH", 3.14 ) ]

        with self.assertRaises(TypeError):
            sunbeam.parse(self.spe3fn, actions)

        with self.assertRaises(ValueError):
            sunbeam.parse(self.spe3fn, "PARSE_RANDOM_SLASH")

    def testData(self):
        regtest = sunbeam.parse(self.REGIONDATA)
        self.assertEqual([3,3,1,2], regtest.props()['OPERNUM'])
