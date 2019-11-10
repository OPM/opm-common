from opm._common import eclArrType
from opm._common import EclFile
from opm._common import ERst

# When extracting the strings from CHAR keywords we get a character array, in
# Python this becomes a list of bytes. This desperate monkey-patching is to
# ensure the EclFile class returns normal Python strings in the case of CHAR
# arrays. The return value is normal Python list of strings.


def getitem_eclfile(self, index):
    
    data = self.__getitem(index)
    array_type = self.arrays[index][1]
    
    if array_type == eclArrType.CHAR:
        return [ x.decode("utf-8") for x in data ]
    
    return data


def get_by_index_erst(self, index, rstep):
    
    array_type = self.get_list_of_arrays(rstep)[index][1]
    data = self.get_erst(index, rstep)
    
    if array_type == eclArrType.CHAR:
        return [ x.decode("utf-8") for x in data ]
    
    return data


def getitem_erst(self, arg):
    
    name = arg[0]
    rstep = arg[1]

    data = self.__getitem(arg)
    array_list = self.get_list_of_arrays(rstep)
    
    array_type=None
    
    for element in array_list:
        if element[0] == name:
            array_type = element[1]
            
    if array_type == eclArrType.CHAR:
        return [ x.decode("utf-8") for x in data ]
    
    return data
        


setattr(EclFile, "__getitem", EclFile.__getitem__)
setattr(EclFile, "__getitem__", getitem_eclfile)
setattr(EclFile, "get", getitem_eclfile)

setattr(ERst, "__getitem", ERst.__getitem__)
setattr(ERst, "__getitem__", getitem_erst)
setattr(ERst, "get_erst", get_by_index_erst)

