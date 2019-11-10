import unittest
import sys
import numpy as np
import io

from opm.io.ecl import ERst, eclArrType
try:
    from tests.utils import test_path
except ImportError:
    from utils import test_path


class TestERst(unittest.TestCase):
    
    def test_reportSteps(self):

        info_str = "List of report steps\n\nseqnum =   37 Date: 06-Jan-2016\nseqnum =   74 Date: 10-Jan-2017\n"

        rst1 = ERst(test_path("data/SPE9.UNRST"))

        self.assertTrue(repr(rst1) == info_str)

        self.assertTrue(rst1.has_report_step_number(37))
        self.assertTrue(rst1.has_report_step_number(74))

        self.assertRaises(ValueError, rst1.load_report_step, 137 );

        rst1.load_report_step(37);
        rst1.load_report_step(74);

        rstep_list = rst1.get_report_steps()
        
        self.assertTrue(37 in rstep_list)
        self.assertTrue(74 in rstep_list)
        
        # __len__ returns number of report steps
        
        self.assertEqual(len(rst1), 2)
        
        
        
    def test_contains(self):

        rst1 = ERst(test_path("data/SPE9.UNRST"))

        self.assertTrue(("PRESSURE", 37) in rst1)
        self.assertTrue(("SWAT", 74) in rst1)
    
        arrayList37=rst1.get_list_of_arrays(37)
    
        self.assertEqual(len(arrayList37), 21)
        
        arrName, arrType, arrSize = arrayList37[16]

        self.assertEqual(arrName, "PRESSURE")
        self.assertEqual(arrType,  eclArrType.REAL)        
        self.assertEqual(arrSize, 9000)
        

    def test_getitem(self):

        rst1 = ERst(test_path("data/SPE9.UNRST"))

        # get first occurrence of ZWEL, report step 37  
        zwel = rst1["ZWEL",37, 0]
        
        self.assertEqual(len(zwel), 78)
        
        self.assertEqual(zwel[0], "INJE1")
        self.assertEqual(zwel[3], "PRODU2")
        self.assertEqual(zwel[6], "PRODU3")
        
        # get first occurrence of INTEHEAD, report step 37  
        inteh = rst1["INTEHEAD",37,0]
        
        self.assertEqual(len(inteh), 411)
        self.assertTrue(isinstance(inteh, np.ndarray))
        self.assertEqual(inteh.dtype, "int32")
        
        self.assertEqual(inteh[1], 201702)
        self.assertEqual(inteh[9], 25)
        self.assertEqual(inteh[64], 6)
        self.assertEqual(inteh[65], 1)
        self.assertEqual(inteh[66], 2016)

        # get first occurrence of PRESSURE, report step 74  
        pres74 = rst1["PRESSURE",74,0]
        
        self.assertTrue(isinstance(pres74, np.ndarray))
        self.assertEqual(pres74.dtype, "float32")
        self.assertEqual(len(pres74), 9000)
        
        self.assertAlmostEqual(pres74[0], 2290.9192, 4)
        self.assertAlmostEqual(pres74[1], 2254.6619, 4)
        self.assertAlmostEqual(pres74[2], 2165.5347, 4)
        self.assertAlmostEqual(pres74[3], 1996.2598, 4)


        xcon = rst1["XCON", 74, 0]
        self.assertTrue(isinstance(xcon, np.ndarray))
        self.assertEqual(xcon.dtype, "float64")
        self.assertEqual(len(xcon), 7540)

        self.assertAlmostEqual(xcon[1], -22.841887080742975, 10)


        logih = rst1["LOGIHEAD", 74, 0]
        self.assertTrue(isinstance(logih, np.ndarray))
        self.assertEqual(len(logih), 121)
        
        for b1, b2 in zip([True, True, False, False, False], logih[0:5]):
            self.assertEqual(b1, b2)
        

    def test_getby_index(self):

        rst1 = ERst(test_path("data/SPE9.UNRST"))

        self.assertTrue(("SGAS", 74) in rst1)
        self.assertTrue(rst1.count("SGAS", 74), 1)
        
        arrayList74=rst1.get_list_of_arrays(74)
        
        ind=-1
        for n,(name, dtype, num) in enumerate(arrayList74):
           if name == "SGAS":
               ind = n     
        
        sgas_a = rst1.get(ind, 74)
        sgas_b = rst1["SGAS", 74, 0]

        self.assertEqual(len(sgas_a), len(sgas_b))
        
        for sg1, sg2 in zip(sgas_a, sgas_b):
            self.assertEqual(sg1, sg2)
            
            
if __name__ == "__main__":

    unittest.main()
    
