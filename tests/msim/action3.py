import sys

def run(ecl_state, schedule, report_step, summary_state):
    P1 = schedule.get_well("P1", report_step)
    P2 = schedule.get_well("P2", report_step)
    P3 = schedule.get_well("P3", report_step)
    P4 = schedule.get_well("P4", report_step)
    sys.stdout.write("Report step {}: BEFORE: P1 Well status: {}\n".format(report_step, P1.status()))
    sys.stdout.write("Report step {}: BEFORE: P2 Well status: {}\n".format(report_step, P2.status()))
    sys.stdout.write("Report step {}: BEFORE: P3 Well status: {}\n".format(report_step, P3.status()))
    sys.stdout.write("Report step {}: BEFORE: P4 Well status: {}\n".format(report_step, P4.status()))

    if report_step == 1:
        schedule.shut_well("P1", report_step)
    elif report_step == 2:
        schedule.shut_well("P2", report_step)
    elif report_step == 3:
        schedule.shut_well("P3", report_step)
    elif report_step == 4:
        schedule.shut_well("P4", report_step)
    elif report_step == 5:
        schedule.open_well("P1", report_step)

    P1 = schedule.get_well("P1", report_step)
    P2 = schedule.get_well("P2", report_step)
    P3 = schedule.get_well("P3", report_step)
    P4 = schedule.get_well("P4", report_step)
    sys.stdout.write("Report step {}: AFTER: P1 Well status: {}\n".format(report_step, P1.status()))
    sys.stdout.write("Report step {}: AFTER: P2 Well status: {}\n".format(report_step, P2.status()))
    sys.stdout.write("Report step {}: AFTER: P3 Well status: {}\n".format(report_step, P3.status()))
    sys.stdout.write("Report step {}: AFTER: P4 Well status: {}\n".format(report_step, P4.status()))    