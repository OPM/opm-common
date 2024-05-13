# The python code in this file does the same as ACT-01 to ACT-05 of opm-tests/actionx/ACTIONX_WCONPROD.DATA
import datetime
import opm_embedded

schedule = opm_embedded.current_schedule

kw = """
EXIT
99 /
"""
schedule.insert_keywords(kw)
