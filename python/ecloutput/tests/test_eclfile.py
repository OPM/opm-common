import sys

try:
    import numpy as np 
    numpyOK=True
except:
    numpyOK=False
    
sys.path.append('../module')

from eclOutput import EclFile

if __name__ == "__main__":
    
    if not numpyOK:
        print ("\nWarning!, numpy package not found found !\n")
        
    file1 = EclFile("../data/SPE9.INIT")
    
    print ("number of arrays: %i " % file1.getNumArrays())
    assert file1.getNumArrays()==24
    
    first = file1.get(0)
    print ("size of first: ",len(first))
    assert len(first)==95

    assert file1.hasArray("PORV")==True
    porv = file1.get("PORV")
    assert len(porv)==9000
    
    if numpyOK:
        assert isinstance(porv, np.ndarray)==True
        assert porv.dtype=="float32"
    else:
        assert isinstance(porv, list)==True

    arrList = file1.getListOfArrays()

    refList=["INTEHEAD","LOGIHEAD","DOUBHEAD","PORV","DEPTH","DX","DY","DZ","PORO","PERMX","PERMY","PERMZ","NTG","TRANX","TRANY","TRANZ","TABDIMS","TAB","ACTNUM","EQLNUM","FIPNUM","PVTNUM","SATNUM","TRANNNC"]

    assert arrList==refList

    doubhead = file1.get("DOUBHEAD")
    assert doubhead[0]==0

    if numpyOK:
        assert isinstance(doubhead, np.ndarray)==True
        assert doubhead.dtype=="float64"
    else:
        assert isinstance(doubhead, list)==True

    tabdims = file1.get("TABDIMS")

    assert tabdims[0]==885
    assert tabdims[7]==67
    assert tabdims[15]==0

    if numpyOK:
        assert isinstance(tabdims, np.ndarray)==True
        assert tabdims.dtype=="int32"
    else:
        assert isinstance(tabdims, list)==True


    file2 = EclFile("../data/9_EDITNNC.SMSPEC")

    assert file2.hasArray("KEYWORDS")==True
    keyw = file2.get("KEYWORDS")
    
    assert len(keyw)==312
    assert keyw[0]=="TIME"
    assert keyw[16]=="FWCT"
    
    file3 = EclFile("../data/9_EDITNNC.INIT")

    assert file3.hasArray("LOGIHEAD")==True
    logih = file3.get("LOGIHEAD")
    assert len(logih)==121
    assert logih[0]==True
    assert logih[2]==False
    assert logih[8]==True
    

    print("Finished, all good")

