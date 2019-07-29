import os
import sys

try:
    import numpy
    numpyOk =True
except:
    numpyOk =False
    

from eclfile_bind import EclFileBind
from egrid_bind import EGridBind
from erst_bind import ERstBind
from erft_bind import ERftBind
from esmry_bind import ESmryBind
from eminit_bind import EclModInitBind
from eclwrite_bind import EclWriteNewBind, EclWriteAppBind

from enum import Enum
import datetime

class eclArrType(Enum):
    INTE = 0
    REAL = 1
    DOUB = 2
    CHAR = 3
    LOGI = 4
    MESS = 5

class EclFile:	

    def __init__(self, fileName):
        self.name = fileName
        self.arrayNameList=[]
        self.arrayTypeList=[]
        
        self.eclfile = EclFileBind(fileName)
        
        arrayList = self.eclfile.getList()

        for name, type, size in arrayList:
            self.arrayNameList.append(name)
            
            if (repr(type)=="eclArrType.INTE"):
                self.arrayTypeList.append(eclArrType.INTE)    
            elif (repr(type)=="eclArrType.REAL"):
                self.arrayTypeList.append(eclArrType.REAL)    
            elif (repr(type)=="eclArrType.DOUB"):
                self.arrayTypeList.append(eclArrType.DOUB)    
            elif (repr(type)=="eclArrType.CHAR"):
                self.arrayTypeList.append(eclArrType.CHAR)    
            elif (repr(type)=="eclArrType.LOGI"):
                self.arrayTypeList.append(eclArrType.LOGI)    
            elif (repr(type)=="eclArrType.MESS"):
                self.arrayTypeList.append(eclArrType.MESS)
            else:
                print("Error, wrong type: %s for array: %s " % (repr(type),name))
                exit(1)
	
    def getListOfArrays(self):
        return self.arrayNameList	

    def getNumArrays(self):
        return len(self.arrayNameList)	

    def hasArray(self, name):
        return self.eclfile.hasKey(name)
    
    def get(self, arg, numpy=True):
        
        try:
            ind = int(arg)
        except:
            if not arg in self.arrayNameList:
                print ("arg: '%s' not found in file %s " % (arg,self.name))
                exit(1)
                       
            ind = self.arrayNameList.index(arg)

        self.eclfile.loadDataByIndex(ind)
        
        if self.arrayTypeList[ind]==eclArrType.INTE:
            if numpy and numpyOk:
                list1=self.eclfile.getInteFromIndexNumpy(ind)
            else:
                list1=self.eclfile.getInteFromIndex(ind)
        elif self.arrayTypeList[ind]==eclArrType.REAL:            
            if numpy and numpyOk:
                list1=self.eclfile.getRealFromIndexNumpy(ind)
            else:
                list1=self.eclfile.getRealFromIndex(ind)
        elif self.arrayTypeList[ind]==eclArrType.DOUB:            
            if numpy and numpyOk:
                list1=self.eclfile.getDoubFromIndexNumpy(ind)
            else:    
                list1=self.eclfile.getDoubFromIndex(ind)
        elif self.arrayTypeList[ind]==eclArrType.LOGI:            
            list1=self.eclfile.getLogiFromIndex(ind)
            
        elif self.arrayTypeList[ind]==eclArrType.CHAR:            
            list1=self.eclfile.getCharFromIndex(ind)
                
        elif self.arrayTypeList[ind]==eclArrType.MESS:            
            list1=[]
        
        return list1


class EGrid:	

    def __init__(self, fileName):
        self.name = fileName
        self.egrid = EGridBind(fileName)
            
    def activeCells(self):
        return self.egrid.activeCells()

    def totalNumberOfCells(self):
        return self.egrid.totalNumberOfCells()

    def global_index(self, i, j, k):
        return self.egrid.global_index(i,j,k)

    def active_index(self, i, j, k):
        return self.egrid.active_index(i,j,k)

    def ijk_from_global_index(self, index):
        ijk_arr=self.egrid.ijk_from_global_index(index)
        return ijk_arr[0],ijk_arr[1],ijk_arr[2]

    def ijk_from_active_index(self, index):
        ijk_arr=self.egrid.ijk_from_active_index(index)
        return ijk_arr[0],ijk_arr[1],ijk_arr[2]

    def dimension(self):
        dims=self.egrid.dimension()
        return dims[0],dims[1],dims[2]

    def getXYZ(self, globInd, numpy=True):

        if numpy and numpyOk:
            X,Y,Z=self.egrid.getCellCornersNumpy(globInd)
        else:
            X,Y,Z=self.egrid.getCellCorners(globInd)
            
        return X,Y,Z

    
class ERst:	

    def __init__(self, fileName):
        self.name = fileName

        self.arrayNameList=[]
        self.arrayTypeList=[]
        self.reportStepNumbers=[]
        
        self.erst = ERstBind(fileName)
        
        arr = self.erst.listOfReportStepNumbers()

        for v in arr:
            self.reportStepNumbers.append(v)
        
        for i,r in enumerate(self.reportStepNumbers):
            self.arrayNameList.append([])
            self.arrayTypeList.append([])
            
            arrayList=self.erst.listOfRstArrays(r)

            for name, type, size in arrayList:
                self.arrayNameList[i].append(name)

                if (repr(type)=="eclArrType.INTE"):
                    self.arrayTypeList[i].append(eclArrType.INTE)    
                elif (repr(type)=="eclArrType.REAL"):
                    self.arrayTypeList[i].append(eclArrType.REAL)    
                elif (repr(type)=="eclArrType.DOUB"):
                    self.arrayTypeList[i].append(eclArrType.DOUB)    
                elif (repr(type)=="eclArrType.CHAR"):
                    self.arrayTypeList[i].append(eclArrType.CHAR)    
                elif (repr(type)=="eclArrType.LOGI"):
                    self.arrayTypeList[i].append(eclArrType.LOGI)    
                elif (repr(type)=="eclArrType.MESS"):
                    self.arrayTypeList[i].append(eclArrType.MESS)
                else:
                    print("Error, wrong type: %s for array: %s " % (repr(type),name))
                    exit(1)

    def hasArray(self, name, num):
        rind=self.reportStepNumbers.index(num)

        if name in self.arrayNameList[rind]:
            return True
        else:
            return False

    def hasReportStepNumber(self, num):
        return self.erst.hasReportStepNumber(num)
    
    def loadReportStepNumber(self, num):
        self.erst.loadReportStepNumber(num)
        
    def listOfReportStepNumbers(self):
        return self.erst.listOfReportStepNumbers()

    def listOfRstArrays(self, num):
        arrayList=self.erst.listOfRstArrays(num)
        return arrayList

    def get(self, name, num, numpy=True):

        if (not num in self.reportStepNumbers):
            print ("\n!Error in get member function.\nReport step number %i not found in %s\n" % (num,self.name))
            exit(1)
        
        rind=self.reportStepNumbers.index(num)
        
        if (not name in self.arrayNameList[rind]):
            print ("\n!Error in get member function.\nArray %s not found for report step number %i in %s\n" % (name, num,self.name))
            exit(1)
        
        aind=self.arrayNameList[rind].index(name)
        
        if self.arrayTypeList[rind][aind]==eclArrType.INTE:
            if numpy and numpyOk:
                array=self.erst.getInteArrayNumpy(name, num)
            else:
                array=self.erst.getInteArray(name, num)
        elif self.arrayTypeList[rind][aind]==eclArrType.LOGI:
            array=self.erst.getLogiArray(name, num)
        elif self.arrayTypeList[rind][aind]==eclArrType.DOUB:
            if numpy and numpyOk:
                array=self.erst.getDoubArrayNumpy(name, num)
            else:
                array=self.erst.getDoubArray(name, num)
        elif self.arrayTypeList[rind][aind]==eclArrType.REAL:
            if numpy and numpyOk:
                array=self.erst.getRealArrayNumpy(name, num)
            else:
                array=self.erst.getRealArray(name, num)
        elif self.arrayTypeList[rind][aind]==eclArrType.CHAR:
            array=self.erst.getCharArray(name, num)
        elif self.arrayTypeList[rind][aind]==eclArrType.MESS:
            array=[]
        else:
            print ("array type not found, ",self.arrayTypeList[rind][aind])
            exit(1)
        
        return array


class ERft:	

    def __init__(self, fileName):
        self.name = fileName

        self.rftReportList=[]

        self.arrayNameList=[]
        self.arrayTypeList=[]
        
        self.erst = ERftBind(fileName)
        rftReportList=self.erst.listOfRftReports()
        
        for r, (welln,(y,m,d)) in enumerate(rftReportList):
            
            dato=datetime.date(y,m,d)
            self.rftReportList.append((welln,dato))
            
            arrList = self.erst.listOfRftArrays(welln,y,m,d)
            
            self.arrayNameList.append([])
            self.arrayTypeList.append([])
            
            for i, (name, type, size) in enumerate(arrList):
                self.arrayNameList[r].append(name)

                if (repr(type)=="eclArrType.INTE"):
                    self.arrayTypeList[r].append(eclArrType.INTE)    
                elif (repr(type)=="eclArrType.REAL"):
                    self.arrayTypeList[r].append(eclArrType.REAL)    
                elif (repr(type)=="eclArrType.DOUB"):
                    self.arrayTypeList[r].append(eclArrType.DOUB)    
                elif (repr(type)=="eclArrType.CHAR"):
                    self.arrayTypeList[r].append(eclArrType.CHAR)    
                elif (repr(type)=="eclArrType.LOGI"):
                    self.arrayTypeList[r].append(eclArrType.LOGI)    
                elif (repr(type)=="eclArrType.MESS"):
                    self.arrayTypeList[r].append(eclArrType.MESS)
                else:
                    print("Error, wrong type: %s for array: %s " % (repr(type),name))
                    exit(1)
                
            
    def getRftReportList(self):
        return self.rftReportList

    def hasRft(self, welln, date):

        report = (welln, date)           
        
        if report in self.rftReportList:
            return True
        else:
            return False

    def hasArray(self, arrName, welln, date):
        dateTuple = (date.year, date.month, date.day)
        return self.erst.hasArray(arrName, welln,dateTuple ) 

    def get(self, arrName, welln, date, numpy=True):
        
        report = (welln,date)
        
        if (not report in self.rftReportList):
            print ("\n!Error in get member function.\nRft Report  [%s, %s]  not found in  %s\n" % (welln, date,self.name))
            exit(1)
        
        rftInd = self.rftReportList.index((welln,date))
        
        if not self.hasArray(arrName, welln, date):
            print ("\n!Error in get member function.\nArray '%s' not found in Rft Report  [%s, %s] in file %s\n" % (arrName, welln, date,self.name))
            exit(1)
        
        arrInd = self.arrayNameList[rftInd].index(arrName)

        if self.arrayTypeList[rftInd][arrInd]==eclArrType.INTE:
            if numpy and numpyOk:
                array=self.erst.getInteRftArrayNumpy(arrName, welln, date.year, date.month, date.day)
            else:    
                array=self.erst.getInteRftArray(arrName, welln, date.year, date.month, date.day)
        elif self.arrayTypeList[rftInd][arrInd]==eclArrType.LOGI:
            array=self.erst.getLogiRftArray(arrName, welln, date.year, date.month, date.day)
        elif self.arrayTypeList[rftInd][arrInd]==eclArrType.DOUB:
            if numpy and numpyOk:
                array=self.erst.getDoubRftArrayNumpy(arrName, welln, date.year, date.month, date.day)
            else:    
                array=self.erst.getDoubRftArray(arrName, welln, date.year, date.month, date.day)
        elif self.arrayTypeList[rftInd][arrInd]==eclArrType.REAL:
            if numpy and numpyOk:
                array=self.erst.getRealRftArrayNumpy(arrName, welln, date.year, date.month, date.day)
            else:    
                array=self.erst.getRealRftArray(arrName, welln, date.year, date.month, date.day)
        elif self.arrayTypeList[rftInd][arrInd]==eclArrType.CHAR:
            array=self.erst.getCharRftArray(arrName, welln, date.year, date.month, date.day)
        elif self.arrayTypeList[rftInd][arrInd]==eclArrType.MESS:
            array=[]
        else:
            print ("array type not found, ",self.arrayTypeList[rind][aind])
            exit(1)
        
        return array
                

class ESmry:	

    def __init__(self, fileName, loadBaseRun=False):
        self.name = fileName
        self.esmry = ESmryBind(fileName,loadBaseRun)

    def hasKey(self, key):
        return self.esmry.hasKey(key)	

    def numberOfVectors(self):
        return self.esmry.numberOfVectors()	

    def keywordList(self):
        return self.esmry.keywordList()	

    def get(self,name, reportStepOnly=False, numpy=True):

        if reportStepOnly:
            if numpy and numpyOk:
                return self.esmry.getAtRstepNumpy(name)
            else: 
                return self.esmry.getAtRstep(name)
        else:
            if numpy and numpyOk:
                return self.esmry.getNumpy(name) 
            else: 
                return self.esmry.get(name)        


class EclModInit:	

    def __init__(self, fileName):
        self.name = fileName
        self.arrayNameList=[]
        self.arrayTypeList=[]

        self.eclmod = EclModInitBind(fileName)

        paramList = self.eclmod.getListOfParameters()

        for name, type in paramList:
            self.arrayNameList.append(name)
            
            if (repr(type)=="eclArrType.INTE"):
                self.arrayTypeList.append(eclArrType.INTE)    
            elif (repr(type)=="eclArrType.REAL"):
                self.arrayTypeList.append(eclArrType.REAL)    
            else:
                print("Error, wrong type: %s for array: %s " % (repr(type),name))
                exit(1)
        
        for param in ["I","J","K","ROW", "COLUMN", "LAYER"]:
            self.arrayNameList.append(param)
            self.arrayTypeList.append(eclArrType.INTE)    
            
        for param in ["PORV","CELLVOL"]:
            self.arrayNameList.append(param)
            self.arrayTypeList.append(eclArrType.REAL)    


    def hasInitReportStep(self):
        return self.eclmod.hasInitReportStep()	

    def hasParameter(self,name):
        return self.eclmod.hasParameter(name)	
    

    def getParam(self, name, numpy=True):

        try:
            ind = self.arrayNameList.index(name)
        except:
            print ("\n!Error, Not possible to get array ",name,"\n")
            exit(1)
            
        if self.arrayTypeList[ind]==eclArrType.INTE:
            if numpy and numpyOk:
               list1=self.eclmod.getInteParamNumpy(name)
            else:    
               list1=self.eclmod.getInteParam(name)
        elif self.arrayTypeList[ind]==eclArrType.REAL:            
            if numpy and numpyOk:
               list1=self.eclmod.getRealParamNumpy(name)
            else:
               list1=self.eclmod.getRealParam(name)
	    
        return list1

        
    def addFilter(self, name, opperator, val):
        
        try:
            ind = self.arrayNameList.index(name)
        except:
            print ("\n!Error, Not possible to get array ",name,"\n")
            exit(1)
            
        if self.arrayTypeList[ind]==eclArrType.INTE:
            self.eclmod.addFilterInteParam(name, opperator, val)
            
        elif self.arrayTypeList[ind]==eclArrType.REAL:            
            self.eclmod.addFilterRealParam(name, opperator, val)

    def resetFilter(self):
        self.eclmod.resetFilter()
        
    def addHCvolFilter(self):
        self.eclmod.addHCvolFilter()
       
    def setDepthFWL(self, fwl):
        self.eclmod.setDepthfwl(fwl)

    def getDims(self):
        return self.eclmod.gridDims()

    def getNumberOfActiveCells(self):
        return self.eclmod.getNumberOfActiveCells()


class EclWrite:	

    def __init__(self, fileName, formatted, newFile = True):
        self.name = fileName
        self.formatted = fileName
        
        if newFile:
            self.eclwrite = EclWriteNewBind(fileName, formatted)
        else:
            self.eclwrite = EclWriteAppBind(fileName, formatted)
            

    def writeArray(self, name, array, arrType="fromFirstElement"):
        
        if (len(array)==0):
            print ("\n!Error, not possible to write array with zero length")
            print ("is member function message what you need ?\n")
            exit(1)

        try:  
            if isinstance(array, numpy.ndarray):

                if (arrType!="fromFirstElement"):
                    print ("!Warning, arrType='%s' is ignored. Data type derived from numpy meta data " % arrType)
            
                if (type(array[0]) is numpy.str_):
                    self.eclwrite.writeString(name, array) 
                elif (type(array[0]) is numpy.bool_):    
                    self.eclwrite.writeBool(name, array) 
                elif (type(array[0]) is numpy.int32) or (type(array[0]) is numpy.int64):    
                    self.eclwrite.writeInteger(name, array) 
                elif (type(array[0]) is numpy.float32):    
                    self.eclwrite.writeFloat(name, array) 
                elif (type(array[0]) is numpy.float64):    
                    self.eclwrite.writeDouble(name, array) 
            
            elif isinstance(array, list):
 
                if (type(array[0]) is int):
                    self.eclwrite.writeInteger(name, array) 
                elif (type(array[0]) is bool):
                    self.eclwrite.writeBool(name, array) 
                elif (type(array[0]) is str):
                    self.eclwrite.writeString(name, array) 
                elif (type(array[0]) is float):
                    
                    if (arrType=="fromFirstElement"):
                        print ("\n!Error, arrType for array '%s' needs to be specified explicitly, float can be both 32 bit (REAL), or 64 bit (DOUB)." % name) 
                        print ("\nExample: writeArray(\"FLOAT32\", arr1, arrType=\"REAL\") or writeArray(\"FLOAT64\", arr1, arrType=\"DOUB\").")
                        print ("\nNotice that using numpy arrays are more safe and therefor recommended \n")
                        exit(1)
                        
                    elif (arrType[0:4].upper()=="DOUB"):                         
                        self.eclwrite.writeDouble(name, array) 
                    elif (arrType[0:4].upper()=="REAL"):                         
                        self.eclwrite.writeFloat(name, array) 
                    else:
                        print ("\nInvalied array type '%s' for array '%s' " % (arrType,name))
                        exit(1)
 
            else:
                print ("\nError, unknown array type, supported array types are numpy and python list \n")
                exit(1)

        except Exception as e: 

            print ("\n!Error with EclWrite class \n")
            print ("\n-----------------------------------------------")
            
            for element in array[1:]:
                if type(element) != type(array[0]):
                    print ("\n > all alenemts are not with same type. Arrays with mixed type elments not supported \n")
                    exit(1)
                    
            for x in array:  
                if x == float('-inf'):
                    print ("\n > -inf value found in array '%s'\n" % name)
                    exit(1)

                if x == float('+inf'):
                    print ("\n > +inf value found in array '%s'\n" % name)
                    exit(1)
                    
 
            print(e)
	    
            exit(1)

        
    def message(self, mess):
        self.eclwrite.message(mess) 


