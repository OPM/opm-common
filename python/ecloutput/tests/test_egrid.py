import sys

try:
    import numpy as np 
    numpyOK=True
except:
    numpyOK=False


sys.path.append('../module')

from eclOutput import EGrid


if __name__ == "__main__":

    if not numpyOK:
        print ("\nWarning!, numpy package not found found !\n")

    grid1 = EGrid("../data/9_EDITNNC.EGRID")
    
    nAct=grid1.activeCells()
    print ("activeCells: ",nAct)
    assert nAct==2794
    
    totCells=grid1.totalNumberOfCells()
    print ("totalNumberOfCells: ",totCells)
    assert totCells==3146

    nI,nJ,nK = grid1.dimension()
    
    assert nI==13
    assert nJ==22
    assert nK==11
    
    print ("Dimmension: %ix%ix%i" % (nI,nJ,nK))

    i,j,k = grid1.ijk_from_global_index(1000)
    
    assert i==12
    assert j==10
    assert k==3
    
    globInd=grid1.global_index(12,10,3)
    print ("global_index: %i" % globInd)
    
    assert globInd==1000
    
    i,j,k = grid1.ijk_from_active_index(1000)

    assert i==1
    assert j==15
    assert k==3

    actInd=grid1.active_index(1,15,3)
    print ("active_index: %i " % actInd)
    
    assert actInd==1000

    
    X,Y,Z=grid1.getXYZ(100)

    if numpyOK:
        assert isinstance(X, np.ndarray)==True
        assert isinstance(Y, np.ndarray)==True
        assert isinstance(Z, np.ndarray)==True

        assert X.dtype=="float64"
        assert Y.dtype=="float64"
        assert Z.dtype=="float64"

    else:
        assert isinstance(X, list)==True
        assert isinstance(Y, list)==True
        assert isinstance(Z, list)==True
    

    Xref=[2899.45166015625,2999.390869140625,2899.45166015625,2999.390869140625,2899.4176237656716,2999.3568089317187,2899.417623015281,2999.356808099622]

    Yref=[2699.973388671875,2699.973388671875,2799.969482421875,2799.969482421875,2699.9818918149376,2699.9818918149376,2799.978009571257,2799.9780095915808]

    Zref=[2565.301025390625,2568.791015625,2564.42822265625,2567.918212890625,2575.29443359375,2578.784423828125,2574.421875,2577.911865234375]

    for i in range(0,8):
        assert X[i]==Xref[i]
        assert Y[i]==Yref[i]
        assert Z[i]==Zref[i]

    print ("finished, all good")


