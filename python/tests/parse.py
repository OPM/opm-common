import unittest
import sunbeam
import os.path

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
        self.norne_fname = os.path.abspath('../../examples/data/norne/NORNE_ATW2013.DATA')

    def test_parse(self):
        spe3 = sunbeam.parse(self.spe3fn)
        self.assertEqual('SPE 3 - CASE 1', spe3.state.title)

    def test_parse_with_recovery(self):
        recovery = [("PARSE_RANDOM_SLASH", sunbeam.action.ignore)]
        spe3 = sunbeam.parse(self.spe3fn, recovery=recovery)

    def test_parse_with_multiple_recoveries(self):
        recoveries = [ ("PARSE_RANDOM_SLASH", sunbeam.action.ignore),
                       ("FOO", sunbeam.action.warn),
                       ("PARSE_RANDOM_TEXT", sunbeam.action.throw) ]

        spe3 = sunbeam.parse(self.spe3fn, recovery=recoveries)

    def test_throw_on_invalid_recovery(self):
        recoveries = [ ("PARSE_RANDOM_SLASH", 3.14 ) ]

        with self.assertRaises(TypeError):
            sunbeam.parse(self.spe3fn, recovery=recoveries)

        with self.assertRaises(TypeError):
            sunbeam.parse(self.spe3fn, recovery="PARSE_RANDOM_SLASH")

    def test_data(self):
        pass
        #regtest = sunbeam.parse(self.REGIONDATA)
        #self.assertEqual([3,3,1,2], regtest.props()['OPERNUM'])

    def test_parse_norne(self):
         state = sunbeam.parse(self.norne_fname, recovery=[('PARSE_RANDOM_SLASH', sunbeam.action.ignore)])
         es = state.state
         self.assertEqual(46, es.grid().getNX())
         self.assertEqual(112, es.grid().getNY())
         self.assertEqual(22, es.grid().getNZ())


if __name__ == "__main__":
    unittest.main()

