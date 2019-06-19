import sys
import datetime

try:
    import numpy as np 
    numpyOK=True
except:
    numpyOK=False

sys.path.append('../module')

from eclOutput import ERft


if __name__ == "__main__":

    if not numpyOK:
        print ("\nWarning!, numpy package not found found !\n")

    rft1 = ERft("../data/9_EDITNNC.RFT")

    dato = datetime.date(2019, 2, 1)

    pressure = rft1.get("PRESSURE", "PROD1", dato)

    pref=[227.70115661621094,226.84425354003906,224.6728973388672,222.3954315185547]

    assert len(pref)==len(pressure)
    
    for i in range(0,len(pressure)):
       assert pref[i]==pressure[i]

    if numpyOK:
        assert pressure.dtype=="float32"
        assert isinstance(pressure, np.ndarray)==True
    else:
        assert isinstance(pressure, list)==True

    
    dato = datetime.date(2019, 2, 1)
    assert rft1.hasRft("PROD1", dato)==True

    dato = datetime.date(2019, 2, 1)
    assert rft1.hasRft("XXXX", dato)==False

    dato = datetime.date(2019, 2, 11)
    assert rft1.hasRft("PROD1", dato)==False

    dato = datetime.date(2019, 2, 1)

    assert rft1.hasArray("PRESSURE", "PROD1", dato)==True
    assert rft1.hasArray("XXXX", "PROD1", dato)==False
    

    rftReportList=rft1.getRftReportList()

    assert len(rftReportList)==2
    
    assert rftReportList[0]==("PROD1",datetime.date(2019,2,1))
    assert rftReportList[1]==("INJ1",datetime.date(2019,2,1))

    kcon = rft1.get("CONKPOS", "PROD1", dato)
    ref_kcon=[ 7, 8, 9, 10]

    assert len(kcon)==len(ref_kcon)

    for i in range(0,len(kcon)):
       assert ref_kcon[i]==kcon[i]
    
    if numpyOK:
        assert kcon.dtype=="int32"
        assert isinstance(kcon, np.ndarray)==True
    else:
        assert isinstance(kcon, list)==True
    
    print ("finished, all good")


