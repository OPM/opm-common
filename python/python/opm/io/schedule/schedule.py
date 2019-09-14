from __future__ import absolute_import

from opm import libopmcommon_python as lib
from ..parser.sunbeam import delegate



@delegate(lib.Connection)
class Connection(object):

    @property
    def pos(self):
        return self.I, self.J, self.K

    def __repr__(self):
        return 'Connection(number = {})'.format(self.number)

    # using the names flowing and closed for functions that test if a well is
    # opened or closed at some point, because we might want to use the more
    # imperative words 'open' and 'close' (or 'shut') for *changing* the status
    # later
    @staticmethod
    def flowing():
        def fn(connection): return connection.state == 'OPEN'
        return fn

    @staticmethod
    def closed():
        def fn(connection): return connection.state == 'SHUT'
        return fn

    @staticmethod
    def auto():
        def fn(connection): return connection.state == 'AUTO'
        return fn

