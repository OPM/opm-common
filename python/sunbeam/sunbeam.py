import libsunbeam as lib

class EclipseState(lib.EclipseState):
    pass

def parse(path):
    return lib.parse(path, lib.ParseContext())
