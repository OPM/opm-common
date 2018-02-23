import unittest
import sunbeam

class TestProps(unittest.TestCase):

    def assertClose(self, expected, observed, epsilon=1e-08):
        diff = abs(expected - observed)
        err_msg = '|%g - %g| = %g > %g' % (expected, observed, diff, epsilon)
        self.assertTrue(diff <= epsilon, msg=err_msg)

    def setUp(self):
        self.spe3 = sunbeam.parse('spe3/SPE3CASE1.DATA').state
        self.props = self.spe3.props()

    def test_repr(self):
        self.assertTrue('Eclipse3DProperties' in repr(self.props))

    def test_contains(self):
        p = self.props
        self.assertTrue('PORO'  in p)
        self.assertFalse('NONO' in p)
        self.assertTrue('PORV' in p) # auto generated

    def test_getitem(self):
        p = self.props
        poro = p['PORO']
        self.assertEqual(324, len(poro))
        self.assertEqual(0.13, poro[0])
        self.assertTrue( 'PERMX' in p )
        px = p['PERMX']
        print(len(px))
        self.assertEqual(324, len(px))

    def test_permx_values(self):
        def md2si(md):
            """millidarcy->SI"""
            return md * 1e-3 * 9.869233e-13
        e3dp  = self.props
        grid  = self.spe3.grid()
        permx = e3dp['PERMX']
        print('set(PERMX) = %s' % set(permx))
        # 130mD, 40mD, 20mD, and 150mD, respectively, top to bottom
        darcys = {0:md2si(130), 1:md2si(40), 2:md2si(20), 3:md2si(150)}
        for i in range(grid.getNX()):
            for j in range(grid.getNY()):
                for k in range(grid.getNZ()):
                    g_idx = grid.globalIndex(i,j,k)
                    perm  = permx[g_idx]
                    darcy = darcys[k]
                    self.assertClose(darcy, perm)

    def test_volume(self):
        e3dp  = self.props
        grid  = self.spe3.grid()
        for i in range(grid.getNX()):
            for j in range(grid.getNY()):
                for k in range(grid.getNZ()):
                    g_idx = grid.globalIndex(i,j,k)
                    exp = 293.3 * 293.3 * 30  # cubicfeet = 73 078.6084 cubic meter
                    exp *= (12*0.0254)**3  # cubic feet to cubic meter
                    if k == 0:
                        self.assertClose(exp, grid.getCellVolume(g_idx))
                    self.assertEqual(grid.getCellVolume(g_idx), grid.getCellVolume(None, i, j, k))


if __name__ == "__main__":
    unittest.main()
