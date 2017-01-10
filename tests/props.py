import unittest
import sunbeam

class TestProps(unittest.TestCase):
    spe3 = None

    def setUp(self):
        if self.spe3 is None:
            self.spe3 = sunbeam.parse('spe3/SPE3CASE1.DATA')
        self.props = self.spe3.props()

    def test_repr(self):
        self.assertTrue('Eclipse3DProperties' in repr(self.props))

    def test_contains(self):
        p = self.props
        self.assertTrue('PORO'  in p)
        self.assertFalse('NONO' in p)
        self.assertFalse('PORV' in p)

    def test_getitem(self):
        p = self.props
        poro = p['PORO']
        self.assertEqual(324, len(poro))
        self.assertEqual(0.13, poro[0])
        self.assertTrue( 'PERMX' in p )
        px = p['PERMX']
        print(len(px))
        self.assertEqual(324, len(px))

    def test_regions(self):
        p = self.props
        reg = p.getRegions('SATNUM')
        self.assertEqual(0, len(reg))
