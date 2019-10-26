
from opm._common import EclFileBind, eclArrType


class EclFile:	

    def __init__(self, fileName):
        self.name = fileName
        self.arrayNameList=[]
        self.arrayTypeList=[]
        
        self.eclfile = EclFileBind(fileName)

        arrayList = self.eclfile.getList()

        for name, type, size in arrayList:
            self.arrayNameList.append(name)

            if type==eclArrType.INTE:
                self.arrayTypeList.append(eclArrType.INTE)    
            elif type==eclArrType.REAL:
                self.arrayTypeList.append(eclArrType.REAL)    
            elif type==eclArrType.DOUB:
                self.arrayTypeList.append(eclArrType.DOUB)    
            elif type==eclArrType.CHAR:
                self.arrayTypeList.append(eclArrType.CHAR)    
            elif type==eclArrType.LOGI:
                self.arrayTypeList.append(eclArrType.LOGI)    
            elif type==eclArrType.MESS:
                self.arrayTypeList.append(eclArrType.MESS)
            else:
                message = "Unknown array type: %s for array: '%s' in file %s" % (repr(type),name, fileName) 
                raise NameError(message)
	
    def getListOfArrays(self):
        return self.arrayNameList	

    def getNumArrays(self):
        return len(self.arrayNameList)	

    def hasArray(self, name):
        return self.eclfile.hasKey(name)
    
    def get(self, arg):
        
        try:
            ind = int(arg)
        except:
            if not arg in self.arrayNameList:
                message = "Array '%s' not found in file %s" % (arg,self.name) 
                raise NameError(message)

            ind = self.arrayNameList.index(arg)

        self.eclfile.loadDataByIndex(ind)
        
        if self.arrayTypeList[ind]==eclArrType.INTE:
            array=self.eclfile.getInteFromIndexNumpy(ind)
        elif self.arrayTypeList[ind]==eclArrType.REAL:            
            array=self.eclfile.getRealFromIndexNumpy(ind)
        elif self.arrayTypeList[ind]==eclArrType.DOUB:            
            array=self.eclfile.getDoubFromIndexNumpy(ind)
        elif self.arrayTypeList[ind]==eclArrType.LOGI:            
            array=self.eclfile.getLogiFromIndex(ind)
        elif self.arrayTypeList[ind]==eclArrType.CHAR:            
            array=self.eclfile.getCharFromIndex(ind)
        elif self.arrayTypeList[ind]==eclArrType.MESS:            
            array=[]
        
        return array

