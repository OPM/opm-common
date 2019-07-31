import sys
import datetime

try:
    import numpy as np 
    numpyOK=True
except:
    numpyOK=False

sys.path.append('../module')

from eclOutput import ESmry

if __name__ == "__main__":

    if not numpyOK:
        print ("\nWarning!, numpy package not found found !\n")

    smry1 = ESmry("../data/9_EDITNNC.SMSPEC")
    
    ref_sos=datetime.datetime(2018, 11,1,0,0,0)
    assert smry1.sos==ref_sos
    

    assert smry1.hasKey("FOPT")==True
    assert smry1.hasKey("WOPR:X-2H")==False
    
    assert smry1.numberOfVectors()==112

    keyList=smry1.keywordList()
    
    assert len(keyList)==112
    
    assert keyList[0]=="BGSAT:11,13,9"
    assert keyList[35]=="WGIRH:INJ1"
    
    time1a = smry1.get("TIME", numpy=False)
    assert len(time1a)==130
    assert isinstance(time1a, list)==True

    time1b = smry1.get("TIME", numpy=False, reportStepOnly=True)
    assert len(time1b)==33
    assert isinstance(time1b, list)==True

    time2a = smry1.get("TIME")
    assert len(time2a)==130

    if numpyOK:
        assert isinstance(time2a, np.ndarray)==True
    else:
        assert isinstance(time2a, list)==True
    
    time2b = smry1.get("TIME", reportStepOnly=True)
    assert len(time2b)==33

    if numpyOK:
        assert isinstance(time2b, np.ndarray)==True
    else:
        assert isinstance(time2b, list)==True
    
    time = smry1.get("TIME")
    fopt = smry1.get("FOPT")
   
    assert time[0]==0.0
    assert fopt[len(fopt)-1]==5648039.5


    smry2 = ESmry("../data/RST1.SMSPEC")
    time = smry2.get("TIME")
    fopt = smry2.get("FOPT")

    assert time[0]==395.0
    assert fopt[0]==2335128.75

    smry3 = ESmry("../data/RST1.SMSPEC",True)
    time = smry3.get("TIME")
    fopt = smry3.get("FOPT")

    assert time[0]==0.0
    assert fopt[len(fopt)-1]==5623955.0

    print ("len fopt : %i " % len(fopt))
    fopt2 = smry3.get("FOPT",True)
    print ("len fopt2: %i " % len(fopt2))
    
    # ---------------------------------

    print ("testing new stuff !")
    
    smry1 = ESmry("../data/9_EDITNNC.SMSPEC")

    time2 = smry1.get("TIME", numpy=True)
    fopt2 = smry1.get("FOPT", numpy=True)

    for i,(t,qot) in enumerate(zip(time2,fopt2)):
        print (i, t, qot)
    
    testTime=883.0
    print ("time=%10.3f | res = %10.4f " % (883.0, smry1.getLinIt("FOPT", testTime, numpy=False)))
    
    
    tVect=[890, 900, 910, 920]
    tVectNp=np.array(tVect) 
    
    isinstance(tVectNp, np.ndarray)==True
    
    resVect1=smry1.getLinIt("FOPT", tVect, numpy=False)
    resVect2=smry1.getLinIt("FOPT", tVect, numpy=True)
    resVect3=smry1.getLinIt("FOPT", tVectNp, numpy=True)

    assert isinstance(resVect1, list)==True
    assert isinstance(resVect2, np.ndarray)==True
    
    print ("\n")

    for t,res1,res2 in zip(tVect, resVect1, resVect2):
        print (t,res1, res2)
    
    print ("finished, all good")
