from __future__ import absolute_import
from os.path import isfile

from opm import libopmcommon_python as lib
from .sunbeam import delegate
from ..schedule import Schedule

@delegate(lib.EclipseState)
class EclipseState(object):
    def __repr__(self):
        return 'EclipseState(title = "%s")' % self.title


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

