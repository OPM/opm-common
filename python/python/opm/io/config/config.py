from __future__ import absolute_import

from opm import libopmcommon_python as lib
from ..parser.sunbeam import delegate


@delegate(lib.SummaryConfig)
class SummaryConfig(object):
    def __repr__(self):
        return 'SummaryConfig()'

