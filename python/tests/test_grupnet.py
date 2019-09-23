import sys
import unittest

import opm.io

class TestGrupnet(unittest.TestCase):

    """
    @classmethod
    def setUpClass(cls):
        norne = 'examples/data/norne/NORNE_ATW2013.DATA'
        cls.sch = opm.io.parse(norne, [('PARSE_RANDOM_SLASH', opm.io.action.ignore)]).schedule

    def test_vfp_table(self):
        self.assertEqual(0, self.sch.group(timestep=0)['PROD'].vfp_table_nr)
        self.assertEqual(9, self.sch.group(timestep=0)['MANI-E2'].vfp_table_nr)
        self.assertEqual(9, self.sch.group(timestep=247)['MANI-E2'].vfp_table_nr)
        self.assertEqual(9999, self.sch.group(timestep=0)['MANI-K1'].vfp_table_nr)
        self.assertEqual(0, self.sch.group(timestep=0)['INJE'].vfp_table_nr)
    """


if __name__ == '__main__':
    unittest.main()
