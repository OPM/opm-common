import sys

try:
    import numpy as np 
    numpyOK=True
except:
    numpyOK=False

sys.path.append('../module')

from eclOutput import EclModInit


if __name__ == "__main__":
    
    if not numpyOK:
        print ("\nWarning!, numpy package not found found !\n")

    init1 = EclModInit("../data/9_EDITNNC")
         
    celvol1= init1.getParam("CELLVOL", numpy=False)
    assert isinstance(celvol1, list)==True

    celvol= init1.getParam("CELLVOL")
    assert len(celvol)==2794
    
    if numpyOK:
        assert isinstance(celvol, np.ndarray)==True
    else:
        assert isinstance(celvol, list)==True
        
    init1.addFilter("CELLVOL","gt",99880)
    celvol= init1.getParam("CELLVOL")
    assert len(celvol)==2672
    
    assert min(celvol) > 99880.0

    assert round(sum(celvol))==266897307.0

    init1.resetFilter()
    
    eqlnum1= init1.getParam("EQLNUM", numpy=False)
    assert isinstance(eqlnum1, list)==True
    
    assert len(eqlnum1)==2794
    assert sum(eqlnum1)==4498 

    eqlnum2= init1.getParam("EQLNUM")

    if numpyOK:
        assert isinstance(eqlnum2, np.ndarray)==True
    else:
        assert isinstance(eqlnum2, list)==True
        
    assert len(eqlnum2)==2794
    assert sum(eqlnum2)==4498 


    print("Finished, all good")

