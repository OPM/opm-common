import sys

sys.path.append('../module')

from eclOutput import EclModInit


if __name__ == "__main__":
    
    init1 = EclModInit("../data/9_EDITNNC")
         
    celvol= init1.getParam("CELLVOL")
    assert len(celvol)==2794
    
    init1.addFilter("CELLVOL","gt",99880)
    celvol= init1.getParam("CELLVOL")
    assert len(celvol)==2672
    
    assert min(celvol) > 99880.0


    assert round(sum(celvol))==266897307.0
    

    print("Finished, all good")

