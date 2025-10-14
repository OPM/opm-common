import unittest
import datetime as dt

from opm.io.parser import Parser
from opm.io.ecl_state import EclipseState
from opm.io.schedule import Schedule

try:
    from tests.utils import test_path
except ImportError:
    from utils import test_path

class TestSchedule(unittest.TestCase):


    @classmethod
    def setUpClass(cls):
        deck  = Parser().parse(test_path('spe3/SPE3CASE1.DATA'))
        state = EclipseState(deck)
        cls.sch = Schedule( deck, state )

    def testWells(self):
        self.assertTrue( isinstance( self.sch.get_wells(0), list) )
        self.assertEqual(2, len(self.sch.get_wells(0)))

        with self.assertRaises(Exception):
            self.sch.get_well('foo', 0)

    def testContains(self):
        self.assertTrue('PROD' in self.sch)
        self.assertTrue('INJ'  in self.sch)
        self.assertTrue('NOT'  not in self.sch)
        self.assertFalse('NOT' in self.sch)

    def testStartEnd(self):
        self.assertEqual(dt.datetime(2015, 1, 1),   self.sch.start)
        self.assertEqual(dt.datetime(2029, 12, 28), self.sch.end)

    def testReportsteps(self):
        reportsteps = self.sch.reportsteps
        self.assertEqual(176, len(reportsteps))
        self.assertEqual(dt.datetime(2016, 1, 1), reportsteps[7])

    def testGroups(self):

        G1 = self.sch[0].group( 'G1')
        self.assertTrue(G1.name == 'G1')
        self.assertTrue(G1.num_wells == 2)

        names = G1.well_names

        self.assertEqual(2, len(names))

        self.assertTrue(self.sch.get_well('INJ', 0).isinjector())
        self.assertTrue(self.sch.get_well('PROD', 0).isproducer())

        with self.assertRaises(Exception):
            self.sch[0].group('foo')

    def test_injection_properties(self):
        deck  = Parser().parse(test_path('spe3/SPE3CASE1.DATA'))
        state = EclipseState(deck)
        sch = Schedule( deck, state )
        report_step = 4
        well_name = 'INJ'
        prop = sch.get_injection_properties(well_name, report_step)
        self.assertEqual(prop['surf_inj_rate'], 4700.0) # Mscf/day
        self.assertEqual(prop['resv_inj_rate'], 0.0) # rb/day
        self.assertEqual(prop['bhp_target'], 4000.0) # psi
        self.assertEqual(prop['thp_target'], 0.0)
        with self.assertRaises(IndexError):
            prop = sch.get_injection_properties("UNDEF", report_step)
        with self.assertRaises(KeyError):
            prop = sch.get_injection_properties("PROD", report_step)

    def test_production_properties(self):
        deck  = Parser().parse(test_path('spe3/SPE3CASE1.DATA'))
        state = EclipseState(deck)
        sch = Schedule( deck, state )
        report_step = 4
        well_name = 'PROD'
        prop = sch.get_production_properties(well_name, report_step)
        self.assertEqual(prop['alq_value'], 0.0)
        self.assertEqual(prop['bhp_target'], 500.0)
        self.assertEqual(prop['gas_rate'], 6200.0)
        self.assertEqual(prop['oil_rate'], 0.0)
        self.assertEqual(prop['water_rate'], 0.0)
        self.assertEqual(prop['liquid_rate'], 0.0)
        self.assertEqual(prop['resv_rate'], 0.0)
        self.assertEqual(prop['thp_target'], 0.0)
        with self.assertRaises(IndexError):
            prop = sch.get_production_properties("UNDEF", report_step)
        with self.assertRaises(KeyError):
            prop = sch.get_production_properties("INJ", report_step)

    def test_well_names(self):
        deck  = Parser().parse(test_path('spe3/SPE3CASE1.DATA'))
        state = EclipseState(deck)
        sch = Schedule( deck, state )
        wnames = sch.well_names("*")
        self.assertTrue("PROD" in wnames)
        self.assertTrue("INJ" in wnames)
        self.assertEqual(len(wnames), 2)


    def test_getitem(self):
        deck  = Parser().parse(test_path('spe3/SPE3CASE1.DATA'))
        state = EclipseState(deck)
        sch = Schedule( deck, state )
        self.assertEqual(len(sch), 176)
        with self.assertRaises(IndexError):
            a = sch[200]

        st100 = sch[100]
        nupcol = st100.nupcol

    def test_open_shut(self):
        deck  = Parser().parse(test_path('spe3/SPE3CASE1.DATA'))
        state = EclipseState(deck)
        sch = Schedule( deck, state )
if __name__ == "__main__":
    unittest.main()
