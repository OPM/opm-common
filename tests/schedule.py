import unittest
import sunbeam

spe3 = sunbeam.parse('spe3/SPE3CASE1.DATA')

class TestSchedule(unittest.TestCase):
    def setUp(self):
        self.sch = spe3.schedule

    def testWells(self):
        self.assertEqual(2, len(self.sch.wells))

    def testContains(self):
        self.assertTrue('PROD' in self.sch)
        self.assertTrue('INJ'  in self.sch)
        self.assertTrue('NOT'  not in self.sch)
        self.assertFalse('NOT' in self.sch)
