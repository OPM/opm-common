from __future__ import absolute_import
from os.path import isfile

from opm import libopmcommon_python as lib
from .sunbeam import delegate


@delegate(lib.SunbeamState)
class SunbeamState(object):

    @property
    def deck(self):
        return self._deck()

    @property
    def summary_config(self):
        return self._summary_config()

