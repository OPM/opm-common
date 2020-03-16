import sys
sys.stdout.write("Running PYACTION\n")
if "FOPR" in context.sim:
    sys.stdout.write("Have FOPR: {}\n".format( context.sim["FOPR"] ))
else:
    sys.stdout.write("Missing FOPR\n")

grid = context.state.grid()
sys.stdout.write("Grid dimensions: ({},{},{})\n".format(grid.nx, grid.ny, grid.nz))

prod_well = context.schedule.get_well("PROD1", context.report_step)
sys.stdout.write("Well status: {}\n".format(prod_well.status()))
if not "list" in context.storage:
    context.storage["list"] = []
context.storage["list"].append(context.report_step)

if context.sim.well_var("PROD1", "WWCT") > 0.80:
    context.schedule.shut_well("PROD1", context.report_step)
    context.schedule.open_well("PROD2", context.report_step)
    context.sim.update("RUN_COUNT", 1)
print(context.storage["list"])

