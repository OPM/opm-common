import sys


sys.path.append('../module')

from eclOutput import ESmry

if __name__ == "__main__":

    smry1 = ESmry("../data/9_EDITNNC.SMSPEC")

    assert smry1.hasKey("FOPT")==True
    assert smry1.hasKey("WOPR:X-2H")==False
   # print smry1.numberOfVectors()
    
    assert smry1.numberOfVectors()==112

    keyList=smry1.keywordList()
    
    assert len(keyList)==112
    
    assert keyList[0]=="BGSAT:11,13,9"
    assert keyList[35]=="WGIRH:INJ1"
    
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

    
    print ("finished, all good")
