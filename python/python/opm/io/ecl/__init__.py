from opm._common import eclArrType
from opm._common import EclFile

import sys
import datetime
import numpy as np

# When extracting the strings from CHAR keywords we get a character array, in
# Python this becomes a list of bytes. This desperate monkey-patching is to
# ensure the EclFile class returns normal Python strings in the case of CHAR
# arrays. The return value is normal Python list of strings.

@property
def eclfile_get_list_of_arrays(self):

    if sys.version_info.major == 2:
        rawData = self.__get_list_of_arrays()
        return [ ( x[0].encode("utf-8"), x[1], x[2] ) for x in rawData ]
    else:
        return self.__get_list_of_arrays()


def getitem_eclfile(self, arg):

    if isinstance(arg, tuple):
        data, array_type = self.__get_data(str(arg[0]), int(arg[1]))
    else:
        data, array_type = self.__get_data(arg)

    if array_type == eclArrType.CHAR:
        return [ x.decode("utf-8") for x in data ]

    return data




setattr(EclFile, "__getitem__", getitem_eclfile)
setattr(EclFile, "arrays", eclfile_get_list_of_arrays)

