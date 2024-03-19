# This script does the same as action_count.py, using the attributes of opm_embedded instead of the run function

import opm_embedded

summary_state = opm_embedded.current_summary_state

if (not 'stop_on_next_run' in locals()): # Define this variable on the first run
    stop_on_next_run = False

if (not stop_on_next_run):
    if not "run_count" in summary_state:
        summary_state["run_count"] = 0
    summary_state["run_count"] += 1

if summary_state.elapsed() > (365 + 15)*24*3600:
    stop_on_next_run = True