import opm_embedded

summary_state = opm_embedded.current_summary_state
schedule = opm_embedded.current_schedule

for well in summary_state.wells:
    schedule.open_well(well, 1000)