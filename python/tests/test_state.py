import unittest
import opm.io

class TestState(unittest.TestCase):

    """
    @classmethod
    def setUpClass(cls):
        cls.spe3 = opm.io.parse('tests/spe3/SPE3CASE1.DATA')
        cpa = opm.io.parse('tests/data/CORNERPOINT_ACTNUM.DATA')
        cls.state = cls.spe3.state
        cls.cp_state = cpa.state
    

    def test_summary(self):
        smry = self.spe3.summary_config
        self.assertTrue('SummaryConfig' in repr(smry))
        self.assertTrue('WOPR' in smry) # hasKeyword
        self.assertFalse('NONO' in smry) # hasKeyword
    
    """


if __name__ == "__main__":
    unittest.main()
