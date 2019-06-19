import sys

sys.path.append('../module')

from eclOutput import ERft


import datetime


if __name__ == "__main__":


    rft1 = ERft("../data/9_EDITNNC.RFT")

    dato = datetime.date(2019, 2, 1)

    pressure = rft1.get("PRESSURE", "PROD1", dato)

    pref=[227.70115661621094,226.84425354003906,224.6728973388672,222.3954315185547]

    assert len(pref)==len(pressure)
    
    for i in range(0,len(pressure)):
       assert pref[i]==pressure[i]


    # testing hasRft
    
    dato = datetime.date(2019, 2, 1)
    assert rft1.hasRft("PROD1", dato)==True

    dato = datetime.date(2019, 2, 1)
    assert rft1.hasRft("XXXX", dato)==False

    dato = datetime.date(2019, 2, 11)
    assert rft1.hasRft("PROD1", dato)==False

    # testing hasArray
    
    dato = datetime.date(2019, 2, 1)

    assert rft1.hasArray("PRESSURE", "PROD1", dato)==True
    assert rft1.hasArray("XXXX", "PROD1", dato)==False
    

    # testing getRftReportList

    rftReportList=rft1.getRftReportList()

    assert len(rftReportList)==2
    
    assert rftReportList[0]==("PROD1",datetime.date(2019,2,1))
    assert rftReportList[1]==("INJ1",datetime.date(2019,2,1))

    
    print ("finished, all good")


