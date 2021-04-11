from __future__ import absolute_import

from opm._common import EModel, eclArrType
from opm._common import calc_cell_vol

def emodel_add_filter(self, key, operator, val1, val2 = None):

    if key in ["I", "J", "K", "ROW", "COLUMN", "LAYER"]:
        arrType=eclArrType.INTE
    else:
        arrList = self.get_list_of_arrays()
        arrNameList = [item[0] for item in arrList]
        arrTypeList = [item[1] for item in arrList]
        
        if key not in arrNameList:
            raise ValueError("{} array not found in EModel".format(key))
        
        ind = arrNameList.index(key)
        
        arrType = arrTypeList[ind]
        
    if not val2:
        if arrType == eclArrType.INTE:
            self.__add_filter(key, operator, int(val1))
        elif arrType == eclArrType.REAL:
            self.__add_filter(key, operator, float(val1))
    else:
        if arrType == eclArrType.INTE:
            self.__add_filter(key, operator, int(val1), int(val2))
        elif arrType == eclArrType.REAL:
            self.__add_filter(key, operator, float(val1), float(val2))

    
setattr(EModel, "add_filter", emodel_add_filter)
