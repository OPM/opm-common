def run(ecl_state, schedule, report_step, summary_state, actionx_callback):
	if (report_step == 2):
		wconprod_shut = """
		WCONPROD
			'{}' '{}' '{}' {} 4* {} /
		/
			""".format('P1', 'SHUT', 'ORAT', 4000, 1000)
		schedule.insert_keywords(wconprod_shut)
		wconprod_open = """
		WCONPROD
			'{}' '{}' '{}' {} 4* {} /
		/
			""".format('P1', 'OPEN', 'ORAT', 4000, 1000)
		schedule.insert_keywords(wconprod_open, report_step + 1)