from opm._common import eclArrType
from opm._common import EclFile
from opm._common import ERst

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


def erst_get_list_of_arrays(self, arg):

    if sys.version_info.major==2:
        rawData = self.__get_list_of_arrays(arg)
        return [ ( x[0].encode("utf-8"), x[1], x[2] ) for x in rawData ]
    else:
        return self.__get_list_of_arrays(arg)


def getitem_erst(self, arg):

    if not isinstance(arg, tuple):
        raise ValueError("expecting tuple argument, (index, rstep), (name, rstep) or (name, rstep, occurrence) ")

    if len(arg) == 2:
        if isinstance(arg[0], int):
            data, array_type = self.__get_data(arg[0], int(arg[1]))
        else:
            data, array_type = self.__get_data(str(arg[0]), int(arg[1]), 0)  # default first occurrence
    elif len(arg) == 3:
        data, array_type = self.__get_data(str(arg[0]), int(arg[1]), int(arg[2]))
    else:
        raise ValueError("expecting tuple argument with 2 or 3 argumens: (index, rstep), (name, rstep) or (name, rstep, occurrence) ")

    if array_type == eclArrType.CHAR:
        return [ x.decode("utf-8") for x in data ]

    return data


def contains_erst(self, arg):

    if isinstance(arg, tuple):
        if len(arg) == 2:
            return self.__contains((arg[0], arg[1]))
        else:
            raise ValueError("expecting tuple (array name , report step number) or \
                              or report step number")

    elif isinstance(arg, int):
        return self.__has_report_step(arg)

    else:
        raise ValueError("expecting tuple (array name , report step number) or \
                          or report step number")



setattr(EclFile, "__getitem__", getitem_eclfile)
setattr(EclFile, "arrays", eclfile_get_list_of_arrays)

setattr(ERst, "__contains__", contains_erst)
setattr(ERst, "arrays", erst_get_list_of_arrays)
setattr(ERst, "__getitem__", getitem_erst)


