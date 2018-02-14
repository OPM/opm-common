from __future__ import absolute_import

from .schedule import Well, Completion, Schedule
from .libsunbeam import action
from .config     import EclipseConfig
from .parser     import parse, parse_string, deck, deck_string


__version__     = '0.0.4'
__license__     = 'GNU General Public License version 3'
