"""
This is the opm_embedded module for embedding python code in PYACTION.
"""

# If you change anything here, please recreate and update the python stub file for opm_embedded as described in python/README.md
from opm.opmcommon_python.embedded import current_ecl_state, current_summary_state, current_schedule, current_report_step
from opm.opmcommon_python import OpmLog
from opm.opmcommon_python import DeckKeyword # Needed for PYINPUT
