# The python code in this file does the same as ACT-01 to ACT-05 of opm-tests/actionx/ACTIONX_WCONPROD.DATA
import opm_embedded

opm_embedded.OpmLog.info("Info from logger.py!")
opm_embedded.OpmLog.warning("Warning from logger.py!")
opm_embedded.OpmLog.error("Error from logger.py!")
opm_embedded.OpmLog.problem("Problem from logger.py!")
opm_embedded.OpmLog.bug("Bug from logger.py!")
opm_embedded.OpmLog.debug("Debug from logger.py!")
opm_embedded.OpmLog.note("Note from logger.py!")
