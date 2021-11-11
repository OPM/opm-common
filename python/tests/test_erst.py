import unittest
import sys
import numpy as np
import io

from opm.io.ecl import ERst, eclArrType, EclOutput

from .utils import test_path


class TestERst(unittest.TestCase):

    def test_reportSteps(self):

        rst1 = ERst(test_path("data/SPE9.UNRST"))

        self.assertTrue( 37 in rst1)
        self.assertTrue( 74 in rst1)

        self.assertRaises(ValueError, rst1.load_report_step, 137 );

        rst1.load_report_step(37);
        rst1.load_report_step(74);

        self.assertTrue(37 in rst1.report_steps)
        self.assertTrue(74 in rst1.report_steps)

        self.assertEqual(len(rst1), 2)


    def test_contains(self):

        rst1 = ERst(test_path("data/SPE9.UNRST"))

        self.assertTrue(("PRESSURE", 37) in rst1)
        self.assertTrue(("SWAT", 74) in rst1)

        arrayList37=rst1.arrays(37)

        self.assertEqual(len(arrayList37), 21)

        arrName, arrType, arrSize = arrayList37[16]

        self.assertEqual(arrName, "PRESSURE")
        self.assertEqual(arrType,  eclArrType.REAL)
        self.assertEqual(arrSize, 9000)


    def test_getitem(self):

        rst1 = ERst(test_path("data/SPE9.UNRST"))

        # get first occurrence of ZWEL, report step 37
        zwel1 = rst1[11, 37]

        zwel2 = rst1["ZWEL",37, 0]
        zwel3 = rst1["ZWEL",37]

        for v1,v2 in zip (zwel1, zwel2):
            self.assertEqual(v1, v2)

        for v1,v2 in zip (zwel1, zwel3):
            self.assertEqual(v1, v2)

        self.assertEqual(len(zwel1), 78)

        self.assertEqual(zwel1[0], "INJE1")
        self.assertEqual(zwel1[3], "PRODU2")
        self.assertEqual(zwel1[6], "PRODU3")

        # get first occurrence of INTEHEAD, report step 37
        inteh = rst1["INTEHEAD",37]

        self.assertEqual(len(inteh), 411)
        self.assertTrue(isinstance(inteh, np.ndarray))
        self.assertEqual(inteh.dtype, "int32")

        self.assertEqual(inteh[1], 201702)
        self.assertEqual(inteh[9], 25)
        self.assertEqual(inteh[64], 6)
        self.assertEqual(inteh[65], 1)
        self.assertEqual(inteh[66], 2016)

        # get first occurrence of PRESSURE, report step 74
        pres74 = rst1["PRESSURE",74]

        self.assertTrue(isinstance(pres74, np.ndarray))
        self.assertEqual(pres74.dtype, "float32")
        self.assertEqual(len(pres74), 9000)

        self.assertAlmostEqual(pres74[0], 2290.9192, 4)
        self.assertAlmostEqual(pres74[1], 2254.6619, 4)
        self.assertAlmostEqual(pres74[2], 2165.5347, 4)
        self.assertAlmostEqual(pres74[3], 1996.2598, 4)

        xcon = rst1["XCON", 74]
        self.assertTrue(isinstance(xcon, np.ndarray))
        self.assertEqual(xcon.dtype, "float64")
        self.assertEqual(len(xcon), 7540)

        self.assertAlmostEqual(xcon[1], -22.841887080742975, 10)

        logih = rst1["LOGIHEAD", 74]
        self.assertTrue(isinstance(logih, np.ndarray))
        self.assertEqual(len(logih), 121)

        for b1, b2 in zip([True, True, False, False, False], logih[0:5]):
            self.assertEqual(b1, b2)


    def test_getby_index(self):

        rst1 = ERst(test_path("data/SPE9.UNRST"))

        self.assertTrue(("SGAS", 74) in rst1)
        self.assertTrue(rst1.count("SGAS", 74), 1)

        arrayList74=rst1.arrays(74)

        array_name_list = [item[0] for item in arrayList74]
        ind =  array_name_list.index("SGAS")

        sgas_a = rst1[ind, 74]
        sgas_b = rst1["SGAS", 74]

        self.assertEqual(len(sgas_a), len(sgas_b))

        for sg1, sg2 in zip(sgas_a, sgas_b):
            self.assertEqual(sg1, sg2)


    def test_list_of_arrays(self):

        refArrList = ["SEQNUM", "INTEHEAD", "LOGIHEAD", "DOUBHEAD", "IGRP", "SGRP", "XGRP", "ZGRP", "IWEL",
                      "SWEL","XWEL","ZWEL", "ICON", "SCON", "XCON", "STARTSOL","PRESSURE", "RS", "SGAS", "SWAT", "ENDSOL"]

        rst1 = ERst(test_path("data/SPE9.UNRST"))

        array_list_74=rst1.arrays(74)

        self.assertEqual(len(refArrList), len(array_list_74) )

        for n, (name, arrType, arrSize) in enumerate(array_list_74):

            self.assertEqual(name, refArrList[n])

            if arrType != eclArrType.MESS:
                array = rst1[name, 74]
                self.assertEqual(len(array), arrSize)

            if arrType == eclArrType.INTE:
                self.assertEqual(array.dtype, "int32")
            elif arrType == eclArrType.REAL:
                self.assertEqual(array.dtype, "float32")
            elif arrType == eclArrType.DOUB:
                self.assertEqual(array.dtype, "float64")
            elif arrType == eclArrType.LOGI:
                self.assertEqual(array.dtype, "bool")
            elif arrType == eclArrType.CHAR:
                self.assertTrue(isinstance(array, list))


    def test_get_occurence(self):

        testFile = test_path("data/TMP.UNRST")

        npArr11 = np.array([11.1, 11.2, 11.3, 11.4], dtype='float32')
        npArr12 = np.array([12.1, 12.2, 12.3, 12.4], dtype='float32')
        npArr13 = np.array([13.1, 13.2, 13.3, 13.4], dtype='float32')

        npArr21 = np.array([21.1, 21.2, 21.3, 21.4], dtype='float32')
        npArr22 = np.array([22.1, 22.2, 22.3, 22.4], dtype='float32')
        npArr23 = np.array([23.1, 23.2, 23.3, 23.4], dtype='float32')

        seqn1 = np.array([1], dtype='int32')
        seqn2 = np.array([2], dtype='int32')

        out1 = EclOutput(testFile)

        out1.write("SEQNUM", seqn1)
        out1.write("TESTING", npArr11)
        out1.write("TESTING", npArr12)
        out1.write("TESTING", npArr13)

        out1.write("SEQNUM", seqn2)
        out1.write("TESTING", npArr21)
        out1.write("TESTING", npArr22)
        out1.write("TESTING", npArr23)

        rst1 = ERst(testFile)

        with self.assertRaises(IndexError):
            arr = rst1["TESTING", 1, 3]
            arr = rst1["TESTING", 2, 3]

        test_npArr11 = rst1["TESTING",1, 0]
        test_npArr12 = rst1["TESTING",1, 1]
        test_npArr13 = rst1["TESTING",1, 2]

        self.assertTrue( np.array_equal(test_npArr11, npArr11) )
        self.assertTrue( np.array_equal(test_npArr12, npArr12) )
        self.assertTrue( np.array_equal(test_npArr13, npArr13) )

        test_npArr21 = rst1["TESTING",2, 0]
        test_npArr22 = rst1["TESTING",2, 1]
        test_npArr23 = rst1["TESTING",2, 2]

        self.assertTrue( np.array_equal(test_npArr21, npArr21) )
        self.assertTrue( np.array_equal(test_npArr22, npArr22) )
        self.assertTrue( np.array_equal(test_npArr23, npArr23) )


if __name__ == "__main__":

    unittest.main()

