# This script does the same as action2.py, using the attributes of opm_embedded instead of the run function

import opm_embedded

summary_state = opm_embedded.current_summary_state
schedule = opm_embedded.current_schedule

for well in summary_state.wells:
    if summary_state.well_var(well, "WWCT") > 0.50:
        schedule.shut_well(well, opm_embedded.current_report_step)