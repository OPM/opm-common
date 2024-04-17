import opm_embedded

schedule = opm_embedded.current_schedule
report_step = opm_embedded.current_report_step

if (report_step == 2):
	invalid_kw = """
	GRID

	PORO
	    1000*0.1 /
	PERMX
	    1000*1 /
	PERMY
	    1000*0.1 /
	PERMZ
	    1000*0.01 /
	"""
	schedule.insert_keywords(invalid_kw)
	