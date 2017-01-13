from os.path import isfile
import libsunbeam as lib
from .sunbeam import delegate
from .schedule import Schedule
from .config import EclipseConfig

@delegate(lib.EclipseState)
class EclipseState(object):
    def __repr__(self):
        return 'EclipseState(title = "%s")' % self.title

    @property
    def schedule(self):
        return Schedule(self._schedule())

    def props(self):
        return Eclipse3DProperties(self._props())

    def grid(self):
        return EclipseGrid(self._grid())

    def cfg(self):
        return EclipseConfig(self._cfg())


@delegate(lib.Eclipse3DProperties)
class Eclipse3DProperties(object):

    def __repr__(self):
        return 'Eclipse3DProperties()'


@delegate(lib.EclipseGrid)
class EclipseGrid(object):

    def getNX(self):
        return self._getXYZ()[0]
    def getNY(self):
        return self._getXYZ()[1]
    def getNZ(self):
        return self._getXYZ()[2]

    def __repr__(self):
        x,y,z = self._getXYZ()
        g = self.cartesianSize()
        na = self.nactive()
        cnt = '(%d, %d, %d)' % (x,y,z)
        if na != g:
            cnt += ', active = %s' % na
        return 'EclipseGrid(%s)' % cnt

def _parse_context(actions):
    ctx = lib.ParseContext()

    if actions is None:
        return ctx

    # this might be a single tuple, in which case we unpack it and repack it
    # into a list. If it's not a tuple we assume it's an iterable and just
    # carry on
    try:
        key, action = actions
    except ValueError:
        pass
    else:
        actions = [(key, action)]

    for key, action in actions:
        ctx.update(key, action)

    return ctx

def parse(deck, actions = None):
    """deck may be a deck string, or a file path"""
    if isfile(deck):
        return EclipseState(lib.parse(deck, _parse_context(actions)))
    return EclipseState(lib.parseData(deck, _parse_context(actions)))
