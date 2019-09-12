from __future__ import absolute_import
from os.path import isfile

from opm import libopmcommon_python as lib
from .sunbeam import delegate
from ..schedule import Schedule

@delegate(lib.EclipseState)
class EclipseState(object):
    def __repr__(self):
        return 'EclipseState(title = "%s")' % self.title

    def props(self):
        return Eclipse3DProperties(self._props())

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

