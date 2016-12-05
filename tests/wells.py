import unittest
import sunbeam

spe3 = sunbeam.parse('spe3/SPE3CASE1.DATA')

class TestWells(unittest.TestCase):
    def setUp(self):
        self.wells = spe3.schedule.wells

    def testWells(self):
        self.assertEqual(2, len(self.wells))

        well = wells
        well.I
        well.I( timestep )
        (I, J, refDepth ) = well.pos( timestep )

        well.at( timestep ).I

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
