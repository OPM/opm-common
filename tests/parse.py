import unittest
import sunbeam

class TestParse(unittest.TestCase):

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
