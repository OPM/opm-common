"""
This is the opm_embedded module for embedding python code in PYACTION.
"""

from opm.opmcommon_python.embedded import current_ecl_state, current_summary_state, current_schedule, current_report_step
from opm.opmcommon_python import OpmLog
from opm.opmcommon_python import DeckKeyword # Needed for PYINPUT
