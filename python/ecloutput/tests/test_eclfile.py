import sys
import numpy as np 

sys.path.append('../module')

#from mylib.eclOutput import EclFile
from eclOutput import EclFile

if __name__ == "__main__":
    
    file1 = EclFile("../data/SPE9.INIT")
    
    print ("number of arrays: %i " % file1.getNumArrays())
    assert file1.getNumArrays()==24

    first = file1.get(0)
    print ("size of first: ",len(first))
    assert len(first)==95


    assert file1.hasArray("PORV")==True
    porv = file1.get("PORV")
    assert len(porv)==9000
    
    assert porv.dtype=="float32"


    arrList = file1.getListOfArrays()

    refList=["INTEHEAD","LOGIHEAD","DOUBHEAD","PORV","DEPTH","DX","DY","DZ","PORO","PERMX","PERMY","PERMZ","NTG","TRANX","TRANY","TRANZ","TABDIMS","TAB","ACTNUM","EQLNUM","FIPNUM","PVTNUM","SATNUM","TRANNNC"]

    assert arrList==refList

    doubhead = file1.get("DOUBHEAD")
    assert doubhead[0]==0

    tabdims = file1.get("TABDIMS")

    assert tabdims[0]==885
    assert tabdims[7]==67
    assert tabdims[15]==0
    

    print("Finished, all good")

