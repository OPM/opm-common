from opm._common import eclArrType
from opm._common import EclFile


# When extracting the strings from CHAR keywords we get a character array, in
# Python this becomes a list of bytes. This desperate monkey-patching is to
# ensure the EclFile class returns normal Python strings in the case of CHAR
# arrays. The return value is normal Python list of strings.

def getitem(self, index):
    data = self.__getitem(index)
    array_type = self.arrays[index][1]
    if array_type == eclArrType.CHAR:
        return [ x.decode("utf-8") for x in data ]

    return data


def make_getitem(cls):
    setattr(cls, "__getitem", cls.__getitem__)
    setattr(cls, "__getitem__", getitem)


make_getitem(EclFile)
