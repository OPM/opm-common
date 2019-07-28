import sys

try:
    import numpy as np 
    numpyOK=True
except:
    numpyOK=False
    
sys.path.append('../module')

from eclOutput import EclWrite

if __name__ == "__main__":
    
    '''
       Almost all platforms represent Python float values as 64-bit “double-precision” values, according to the IEEE 754 standard.    
       
       https://realpython.com/python-data-types/

       float	32 bits	-3.4E+38 to +3.4E+38
       double	64 bits	-1.7E+308 to +1.7E+308
    

    '''
    
   # intTest=np.array([1,2,3])
   # print (type(intTest[0])) 
   # > default is numpy.int64 if not explicit asking for numpy.int32 
   #  >     npInt32Arr=np.array([1,2,3,4], dtype='int32')


    # test numpy arrays 
    
    npArr1=np.array([1.1, 2.2e+28, 3.3, 4.4], dtype='float32')
    npArr2=np.array([11.11, 12.12, 13.13, 14.14], dtype='float64')

    npArr3=np.array([1,2,3,4], dtype='int32')
    npArr4=np.array([11,12,13,14], dtype='int64')

    npArr5=np.array(["PROD1","","INJ1"], dtype='str')
    npArr6=np.array([True, True, False, True, False, False], dtype='bool')

    file1 = EclWrite("../data/RESULT.DAT", formatted=True, newFile=True)
    file1.writeArray("NP_ARR1", npArr1)
    file1.writeArray("NP_ARR2", npArr2)
    file1.writeArray("NP_ARR3", npArr3)
    file1.writeArray("NP_ARR4", npArr4)
    file1.writeArray("NP_ARR5", npArr5)
    file1.writeArray("NP_ARR6", npArr6)
    


    Arr1=[1.1, 2.2e+28, 3.3, 4.4]
    Arr2=[11.11, 12.12, 13.13, 14.14]

    Arr3=[1,2,3,4]
    Arr4=[11,12,13,14]

    Arr5=["PROD1","","INJ1"]
    Arr6=[True, True, False, True, False, False]

    file2 = EclWrite("../data/RESULT.DAT", formatted=True, newFile=False)
    file2.writeArray("ARR1", Arr1, arrType="REAL")
    file2.writeArray("ARR2", Arr2, arrType="DOUB")
    file2.writeArray("ARR3", Arr3)
    file2.writeArray("ARR4", Arr4)
    file2.writeArray("ARR5", Arr5)
    file2.writeArray("ARR6", Arr6)


    

    print("Finished, all good")

