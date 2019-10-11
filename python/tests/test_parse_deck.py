import unittest
import json
import opm
import opm.io
import os.path

from opm.io.parser import Parser, ParseContext

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
        with self.assertRaises(ValueError):
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



if __name__ == "__main__":
    unittest.main()
