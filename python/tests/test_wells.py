import unittest
import opm
from opm.io import parse

class TestWells(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.sch = parse('tests/spe3/SPE3CASE1.DATA').schedule
        cls.timesteps = cls.sch.timesteps
#        cls.wells = cls.sch.wells

    def inje(self, wells):
        return next(iter(filter(opm.io.schedule.Well.injector(), wells)))

    def prod(self, wells):
        return next(iter(filter(opm.io.schedule.Well.producer(), wells)))

    def testWellPos0(self):
        wells = self.sch.get_wells(0)
        well = wells[0]
        i, j, refdepth = well.pos()

        self.assertEqual(6, i)
        self.assertEqual(6, j)
        self.assertEqual(2247.9, refdepth)

    def testWellStatus(self):
        wells = self.sch.get_wells(0)
        for well in wells:
            self.assertEqual("OPEN", well.status())

    def testGroupName(self):
        wells = self.sch.get_wells(0)
        for well in wells:
            self.assertEqual("G1", well.group())

    def testPreferredPhase(self):
        wells = self.sch.get_wells(0)
        inje, prod = self.inje(wells), self.prod(wells)
        self.assertEqual("GAS", inje.preferred_phase)
        self.assertEqual("GAS", prod.preferred_phase)

    def testGuideRate(self):
        wells = self.sch.get_wells(1)
        inje, prod = self.inje(wells), self.prod(wells)
        self.assertEqual(-1.0, inje.guide_rate())
        self.assertEqual(-1.0, prod.guide_rate())

        wells = self.sch.get_wells(14)
        inje, prod = self.inje(wells), self.prod(wells)
        self.assertEqual(-1.0, inje.guide_rate())
        self.assertEqual(-1.0, prod.guide_rate())

    def testGroupControl(self):
        wells = self.sch.get_wells(1)
        inje, prod = self.inje(wells), self.prod(wells)
        self.assertTrue(inje.available_gctrl())
        self.assertTrue(prod.available_gctrl())

        wells = self.sch.get_wells(14)
        inje, prod = self.inje(wells), self.prod(wells)
        self.assertTrue(inje.available_gctrl())
        self.assertTrue(prod.available_gctrl())

    def testWellDefinedFilter(self):
        defined0 = list(filter(opm.io.schedule.Well.defined(0), self.sch.get_wells(0) ))
        defined1 = list(filter(opm.io.schedule.Well.defined(1), self.sch.get_wells(1) ))
        self.assertEqual(len(list(defined0)), 2)
        self.assertEqual(len(list(defined1)), 2)

    def testWellProdInjeFilter(self):
        inje = list(filter(opm.io.schedule.Well.injector(), self.sch.get_wells(0) ))
        prod = list(filter(opm.io.schedule.Well.producer(), self.sch.get_wells(0) ))

        self.assertEqual(len(inje), 1)
        self.assertEqual(len(prod), 1)

        self.assertEqual(inje[0].name, "INJ")
        self.assertEqual(prod[0].name, "PROD")

    def testOpenFilter(self):
        wells = self.sch.get_wells(1)

        open_at_1 = opm.io.schedule.Well.flowing()
        flowing = list(filter(open_at_1, wells))
        closed  = list(filter(lambda well: not open_at_1(well), wells))

        self.assertEqual(2, len(flowing))
        self.assertEqual(0, len(closed))

        flowing1 = filter(lambda x: not opm.io.schedule.Well.closed()(x), wells)
        closed1  = filter(opm.io.schedule.Well.closed(), wells)
        self.assertListEqual(list(closed), list(closed1))

    def testCompletions(self):
        num_steps = len( self.sch.timesteps )
        w0 = self.sch.get_wells(num_steps - 1)[0]
        c0,c1 = w0.connections()
        self.assertEqual((6,6,2), c0.pos)
        self.assertEqual((6,6,3), c1.pos)


if __name__ == "__main__":
    unittest.main()
