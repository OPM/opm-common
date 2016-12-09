import unittest
import sunbeam

spe3 = sunbeam.parse('spe3/SPE3CASE1.DATA')

class TestWells(unittest.TestCase):
    def setUp(self):
        self.wells = spe3.schedule.wells

    def testWellPos0(self):
        well = self.wells[0]
        i, j, refdepth = well.pos(0)

        self.assertEqual(6, i)
        self.assertEqual(6, j)
        self.assertEqual(2247.9, refdepth)

    def testWellProperty(self):
        well = self.wells[0]
        i, j, refdepth = well.pos

        self.assertEqual(6, i)
        self.assertEqual(6, j)
        self.assertEqual(2247.9, refdepth)
