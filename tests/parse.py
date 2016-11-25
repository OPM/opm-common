import unittest
import sunbeam

class TestParse(unittest.TestCase):

    def setUp(self):
        self.spe3 = 'spe3/SPE3CASE1.DATA'

    def parse(self):
        sunbeam.parse(self.spe3)

    def testParse(self):
        spe3 = sunbeam.parse(self.spe3)
        self.assertEqual('SPE 3 - CASE 1', spe3.title)
