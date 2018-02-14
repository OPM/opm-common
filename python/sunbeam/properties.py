from __future__ import absolute_import
from os.path import isfile

from sunbeam import libsunbeam as lib
from .sunbeam import delegate
from .schedule import Schedule
from .config import EclipseConfig

@delegate(lib.EclipseState)
class EclipseState(object):
    def __repr__(self):
        return 'EclipseState(title = "%s")' % self.title

    def props(self):
        return Eclipse3DProperties(self._props())

    def grid(self):
        return EclipseGrid(self._grid())

    def cfg(self):
        return EclipseConfig(self._cfg())

    @property
    def table(self):
        return Tables(self._tables())

    def faults(self):
        """Returns a map from fault names to list of (i,j,k,D) where D ~ 'X+'"""
        fs = {}
        for fn in self.faultNames():
            fs[fn] = self.faultFaces(fn)
        return fs


@delegate(lib.Eclipse3DProperties)
class Eclipse3DProperties(object):

    def __repr__(self):
        return 'Eclipse3DProperties()'


@delegate(lib.Tables)
class Tables(object):

    def __repr__(self):
        return 'Tables()'

    def _eval(self, x, table, col_name, tab_idx = 0):
        return self._evaluate(table, tab_idx, col_name, x)

    def __getitem__(self, tab_name):
        col_name = None
        if isinstance(tab_name, tuple):
            tab_name, col_name = tab_name

        tab_name = tab_name.upper()
        if not tab_name in self:
            raise ValueError('Table "%s" not in deck.' % tab_name)

        if col_name is None:
            def t_eval(col_name, x, tab_idx = 0):
                return self._eval(x, tab_name, col_name.upper(), tab_idx)
            return t_eval

        col_name = col_name.upper()
        def t_eval(x, tab_idx = 0):
            return self._eval(x, tab_name, col_name, tab_idx)
        return t_eval



@delegate(lib.EclipseGrid)
class EclipseGrid(object):

    def getNX(self):
        return self._getXYZ()[0]
    def getNY(self):
        return self._getXYZ()[1]
    def getNZ(self):
        return self._getXYZ()[2]

    def getCellVolume(self, global_idx=None, i_idx=None, j_idx=None, k_idx=None):
        if global_idx is not None:
            if set([i_idx, j_idx, k_idx]) != set([None]):
                raise ValueError('Specify exactly one of global and all three i,j,k.')
            return self._cellVolume1G(global_idx)
        if None in [i_idx, j_idx, k_idx]:
            raise ValueError('If not global_idx, need all three of i_idx, j_idx, and k_idx.')
        return self._cellVolume3(i_idx, j_idx, k_idx)

    def eclGrid(self):
        return self._ecl_grid_ptr()

    def __repr__(self):
        x,y,z = self._getXYZ()
        g = self.cartesianSize()
        na = self.nactive()
        cnt = '(%d, %d, %d)' % (x,y,z)
        if na != g:
            cnt += ', active = %s' % na
        return 'EclipseGrid(%s)' % cnt


@delegate(lib.SunbeamState)
class SunbeamState(object):

    @property
    def state(self):
        return EclipseState(self._state())


    @property
    def schedule(self):
        return Schedule(self._schedule())


    @property
    def deck(self):
        return self._deck()

    @property
    def summary_config(self):
        return self._summary_config()

