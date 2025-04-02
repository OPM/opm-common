
import unittest
import sys
import numpy as np

from opm.util import EModel
try:
    from tests.utils import test_path
except ImportError:
    from utils import test_path


class TestEModel(unittest.TestCase):

    def test_open_model(self):

        refArrList = ["PORV", "CELLVOL", "DEPTH", "DX", "DY", "DZ", "PORO", "PERMX", "PERMY", "PERMZ", "NTG", "TRANX",
                      "TRANY", "TRANZ", "ACTNUM", "ENDNUM", "EQLNUM", "FIPNUM", "FLUXNUM", "IMBNUM", "PVTNUM",
                      "SATNUM", "SWL", "SWCR", "SGL", "SGU", "ISWL", "ISWCR", "ISGL", "ISGU", "PPCW", "PRESSURE",
                      "RS", "RV", "SGAS", "SWAT", "SOMAX", "SGMAX"]


        self.assertRaises(RuntimeError, EModel, "/file/that/does_not_exists")

        self.assertRaises(ValueError, EModel, test_path("data/9_EDITNNC.EGRID"))
        self.assertRaises(ValueError, EModel, test_path("data/9_EDITNNC.UNRST"))

        mod1 = EModel(test_path("data/9_EDITNNC.INIT"))

        arrayList = mod1.get_list_of_arrays()

        for n, element in enumerate(arrayList):
            self.assertEqual(element[0], refArrList[n])

        celvol1 = mod1.get("CELLVOL")
        self.assertEqual(len(celvol1), 2794)


    def test_add_filter(self):

        mod1 = EModel(test_path("data/9_EDITNNC.INIT"))

        celvol1 = mod1.get("CELLVOL")
        depth1 = mod1.get("DEPTH")

        self.assertTrue(isinstance(celvol1, np.ndarray))
        self.assertEqual(celvol1.dtype, "float32")

        refVol1 = 2.79083e8

        # The algorithm for sum in Python 3.12 changed to Neumaier summation
        # which breaks the following test. Using numpy.sum now to get the
        # old behaviour. For 3.11 they yield the same result.
        self.assertTrue( abs((np.sum(celvol1) - refVol1)/refVol1) < 1.0e-5)

        mod1.add_filter("EQLNUM","eq", 1);
        mod1.add_filter("DEPTH","lt", 2645.21);

        refVol2 = 1.08876e8
        refPorvVol2 = 2.29061e7

        porv2 = mod1.get("PORV")
        celvol2 = mod1.get("CELLVOL")

        self.assertTrue( abs((sum(celvol2) - refVol2)/refVol2) < 1.0e-5)
        self.assertTrue( abs((sum(porv2) - refPorvVol2)/refPorvVol2) < 1.0e-5)

        mod1.reset_filter()
        mod1.add_filter("EQLNUM","eq", 2);
        mod1.add_filter("DEPTH","in", 2584.20, 2685.21);

        refPorvVol3 = 3.34803e7
        porv3 = mod1.get("PORV")

        self.assertTrue( abs((sum(porv3) - refPorvVol3)/refPorvVol3) < 1.0e-5)

        mod1.reset_filter()
        mod1.add_filter("I","lt", 10);
        mod1.add_filter("J","between", 3, 15);
        mod1.add_filter("K","between", 2, 9);

        poro = mod1.get("PORO")

        self.assertEqual(len(poro), 495)


    def test_paramers(self):

        mod1 = EModel(test_path("data/9_EDITNNC.INIT"))

        self.assertFalse("XXX" in mod1)
        self.assertTrue("PORV" in mod1)
        self.assertTrue("PRESSURE" in mod1)
        self.assertTrue("RS" in mod1)
        self.assertTrue("RV" in mod1)

        self.assertEqual(mod1.active_report_step(), 0)


        rsteps = mod1.get_report_steps()
        self.assertEqual(rsteps, [0, 4, 7, 10, 15, 20, 27, 32, 36, 39])

        mod1.set_report_step(7)

        # parameter RS and RV is missing in report step number 7

        self.assertFalse("RS" in mod1)
        self.assertFalse("RV" in mod1)

        mod1.set_report_step(15)

        self.assertTrue("RS" in mod1)
        self.assertTrue("RV" in mod1)

        arrayList = mod1.get_list_of_arrays()


    def test_rsteps_steps(self):

        pres_ref_4_1_10 = [272.608, 244.461, 228.503, 214.118, 201.147, 194.563, 178.02, 181.839, 163.465, 148.677]

        mod1 = EModel(test_path("data/9_EDITNNC.INIT"))

        mod1.add_filter("I","eq", 4);
        mod1.add_filter("J","eq", 1);
        mod1.add_filter("K","eq", 10);

        self.assertTrue(mod1.has_report_step(4))
        self.assertFalse(mod1.has_report_step(2))

        rsteps = mod1.get_report_steps()

        for n, step in enumerate(rsteps):
            mod1.set_report_step(step)
            pres = mod1.get("PRESSURE")

            self.assertTrue(abs(pres[0] - pres_ref_4_1_10[n])/pres_ref_4_1_10[n] < 1.0e-5)


    def test_grid_props(self):

        mod1 = EModel(test_path("data/9_EDITNNC.INIT"))
        nI,nJ,nK = mod1.grid_dims()

        self.assertEqual((nI,nJ,nK), (13, 22, 11))

        nAct = mod1.active_cells()
        self.assertEqual(nAct, 2794)


    def test_hc_filter(self):

        nAct_hc_eqln1 = 1090
        nAct_hc_eqln2 = 1694

        mod1 = EModel(test_path("data/9_EDITNNC.INIT"))

        porv = mod1.get("PORV")

        mod1.set_depth_fwl([2645.21, 2685.21])

        mod1.add_hc_filter()

        porv = mod1.get("PORV")
        self.assertEqual(len(porv), nAct_hc_eqln1 + nAct_hc_eqln2)

        mod1.reset_filter()
        mod1.add_filter("EQLNUM","eq", 1);
        mod1.add_filter("DEPTH","lt", 2645.21);

        porv1 = mod1.get("PORV")
        self.assertEqual(len(porv1), nAct_hc_eqln1)

        mod1.reset_filter()
        mod1.add_filter("EQLNUM","eq", 2);
        mod1.add_filter("DEPTH","lt", 2685.21);

        porv2 = mod1.get("PORV")
        self.assertEqual(len(porv2), nAct_hc_eqln2)

        ivect = mod1.get("I")


if __name__ == "__main__":

    unittest.main()

