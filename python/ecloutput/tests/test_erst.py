import sys


sys.path.append('../module')

from eclOutput import ERst



if __name__ == "__main__":

    rst1 = ERst("../data/SPE9.UNRST")

    assert rst1.hasReportStepNumber(37)==True
    
    # not needed since load on the demand in place also for restart
    #rst1.loadReportStepNumber(37)

    rstNumList=rst1.listOfReportStepNumbers()
    
    assert len(rstNumList)==2

    assert rstNumList[0]==37
    assert rstNumList[1]==74
    
    arrayList37=rst1.listOfRstArrays(37)
    
    assert len(arrayList37)==21
    
    test1 = rst1.get("ZWEL",37)
    assert len(test1)==78
    assert test1[0]=="INJE1"
    assert test1[3]=="PRODU2"
    
    test2 = rst1.get("INTEHEAD",37)
    assert len(test2)==411
    assert test2[1]==201702
    assert test2[9]==25
    assert test2[64]==6
    assert test2[65]==1
    assert test2[66]==2016

    test3 = rst1.get("LOGIHEAD",37)
    assert len(test3)==121
    assert test3[0]==True
    assert test3[1]==True
    assert test3[2]==False

    test4 = rst1.get("DOUBHEAD",37)
    assert len(test4)==229
    assert test4[0]==370.0
    assert test4[101]==0.025
    assert test4[198]==1e+20

    assert rst1.hasArray("PRESSURE", 37)==True
    assert rst1.hasArray("XXX", 37)==False

    test5 = rst1.get("PRESSURE",37)
    assert len(test5)==9000
    assert test5[20]==3666.396240234375
    assert test5[107]==2397.0341796875
    assert test5[1499]==2685.482421875


    print ("finished, all good")


