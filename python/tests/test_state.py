import unittest
import opm.io

class TestState(unittest.TestCase):

    
    @classmethod
    def setUpClass(cls):
        cls.spe3 = opm.io.parse('tests/spe3/SPE3CASE1.DATA')
        cpa = opm.io.parse('tests/data/CORNERPOINT_ACTNUM.DATA')
    

    def test_summary(self):
        smry = self.spe3.summary_config
        self.assertTrue('SummaryConfig' in repr(smry))
        self.assertTrue('WOPR' in smry) # hasKeyword
        self.assertFalse('NONO' in smry) # hasKeyword
    
    


if __name__ == "__main__":
    unittest.main()
