import unittest
import datetime as dt

import opm.parser.schedule
from opm.parser import parse

class TestSchedule(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.sch = parse('tests/spe3/SPE3CASE1.DATA').schedule

    def testWells(self):
        self.assertEqual(2, len(self.sch.get_wells(0)))

        with self.assertRaises(KeyError):
            self.sch.get_well('foo', 0)

    def testContains(self):
        self.assertTrue('PROD' in self.sch)
        self.assertTrue('INJ'  in self.sch)
        self.assertTrue('NOT'  not in self.sch)
        self.assertFalse('NOT' in self.sch)

    def testStartEnd(self):
        self.assertEqual(dt.datetime(2015, 1, 1),   self.sch.start)
        self.assertEqual(dt.datetime(2029, 12, 28), self.sch.end)

    def testTimesteps(self):
        timesteps = self.sch.timesteps
        self.assertEqual(176, len(timesteps))
        self.assertEqual(dt.datetime(2016, 1, 1), timesteps[7])

    def testGroups(self):
        g1 = self.sch.group(0)['G1'].wells
        self.assertEqual(2, len(g1))

        def head(xs): return next(iter(xs))

        inje = head(filter(opm.parser.schedule.Well.injector(), g1))
        prod = head(filter(opm.parser.schedule.Well.producer(), g1))

        self.assertEqual(self.sch.get_well('INJ', 0).isinjector(),  inje.isinjector())
        self.assertEqual(self.sch.get_well('PROD', 0).isproducer(), prod.isproducer())

        with self.assertRaises(KeyError):
            self.sch.group(0)['foo']


if __name__ == "__main__":
    unittest.main()
