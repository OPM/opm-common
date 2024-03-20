def run(ecl_state, schedule, report_step, summary_state, actionx_callback):

    if report_step == 1:
        actionx_callback("CLOSEWELL", ["P1"])
    elif report_step == 2:
        actionx_callback("CLOSEWELL", ["P2"])
    elif report_step == 3:
        actionx_callback("CLOSEWELL", ["P3"])
    elif report_step == 4:
        actionx_callback("CLOSEWELL", ["P4"])
    elif report_step == 5:
        actionx_callback("OPENWELL", ["P1"])