# This is the entry point were all the pybind11/C++ symbols are imported into
# Python. Before actually being used the symbols are typically imported one
# more time to a more suitable location; e.g the Parser() class is imported in
# the opm/io/parser/__init__.py file as:
#
#   from opm._common import Parser
#
# So that end user code can import it as:
#
#   from opm.io.parser import Parser
from __future__ import absolute_import
from .libopmcommon_python import action

from .libopmcommon_python import Parser, ParseContext
from .libopmcommon_python import EclipseState

#from .schedule            import Well, Connection, Schedule
#from .config     import EclipseConfig
#from .parser     import parse, parse_string
