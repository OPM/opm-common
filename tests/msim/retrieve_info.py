import opm_embedded

schedule = opm_embedded.current_schedule
ecl_state = opm_embedded.current_ecl_state
summary_state = opm_embedded.current_summary_state

# Get info from the Schedule Object.
start = schedule.start
end = schedule.end
timesteps = schedule.timesteps
length = schedule.__len__()
injection_properties = schedule.get_injection_properties("INJ",1)
production_properties = schedule.get_production_properties("PROD1",1)
well_names = schedule.well_names("PROD*")
has_well_PROD1 = "PROD1" in schedule
groups = schedule._groups(1)

for group in groups:
	group_name = group.name
	group_num_wells = group.num_wells
	group_well_names = group.well_names

if has_well_PROD1:
	well = schedule.get_well("PROD1",1)
	well_pos = well.pos()
	well_status = well.status()
	well_isdefined = well.isdefined(1)
	well_isinjector = well.isinjector()
	well_isproducer = well.isproducer()
	well_group = well.group()
	well_guide_rate = well.guide_rate()
	well_available_gctrl = well.available_gctrl()
	well_connections = well.connections()
	for well_connection in well_connections:
		well_connection_direction = well_connection.direction
		well_connection_state = well_connection.state
		well_connection_i = well_connection.i
		well_connection_j = well_connection.j
		well_connection_k = well_connection.k
		well_connection_pos = well_connection.pos
		well_connection_attached_to_segment = well_connection.attached_to_segment
		well_connection_center_depth = well_connection.center_depth
		well_connection_rw = well_connection.rw
		well_connection_complnum = well_connection.complnum
		well_connection_number = well_connection.number
		well_connection_sat_table_id = well_connection.sat_table_id
		well_connection_segment_number = well_connection.segment_number
		well_connection_cf = well_connection.cf
		well_connection_kh = well_connection.kh

schedule_state= schedule[1]
schedule_state_nupcol = schedule_state.nupcol
schedule_state_group = schedule_state.group("FIELD")
field_group_name = schedule_state_group.name
field_group_num_wells = schedule_state_group.num_wells
field_group_well_names = schedule_state_group.well_names

all_wells = schedule.get_wells(1)
# This was all from the Schedule Object.

# Get info from the EclipseState Object.
field_props = ecl_state.field_props()

field_props_contains = "PERMX" in field_props
if field_props_contains:
	field_props_getitem = field_props["PERMX"]
field_props_get_double_array = field_props.get_double_array("PERMX")
field_props_get_int_array = field_props.get_int_array("SATNUM")

grid = ecl_state.grid()
grid_getXYZ = grid._getXYZ()
grid_nx = grid.nx
grid_ny = grid.ny
grid_nz = grid.nz
grid_nactive = grid.nactive
grid_cartesianSize = grid.cartesianSize
grid_globalIndex = grid.globalIndex(1,2,3)
grid_getIJK = grid.getIJK(42)
grid_getCellVolume = grid.getCellVolume()
grid_getCellVolume = grid.getCellVolume()
grid_getCellVolume = grid.getCellVolume()
grid_getCellVolume = grid.getCellVolume()
grid_getCellDepth = grid.getCellDepth()
grid_getCellDepth = grid.getCellDepth()
grid_getCellDepth = grid.getCellDepth()
grid_getCellDepth = grid.getCellDepth()

has_input_nnc = ecl_state.has_input_nnc()

input_nnc = ecl_state.input_nnc()

faultNames = ecl_state.faultNames()

for fault in faultNames:
	faultFaces = ecl_state.faultFaces(fault)

jfunc = ecl_state.jfunc()

simulation_config = ecl_state.simulation()

simulation_config_hasThresholdPressure = simulation_config.hasThresholdPressure()
simulation_config_useCPR = simulation_config.useCPR()
simulation_config_useNONNC = simulation_config.useNONNC()
simulation_config_hasDISGAS = simulation_config.hasDISGAS()
simulation_config_hasDISGASW = simulation_config.hasDISGASW()
simulation_config_hasVAPOIL = simulation_config.hasVAPOIL()
simulation_config_hasVAPWAT = simulation_config.hasVAPWAT()
# This was all from the EclipseState Object.

# Get info from the SummaryState Object.
if ("FUOPR" in summary_state):
	FUOPR = summary_state["FUOPR"]
	summary_state["FUOPR"] = 2.0
	summary_state.update("FUOPR", 3.0)

summary_state.update_well_var("PROD1", "WGOR", 0.0)
summary_state_groups = summary_state.groups
for group in summary_state_groups:
	if (summary_state.has_group_var(group, "GOPR")):
		summary_state_group_var = summary_state.group_var(group, "GOPR")
		summary_state_update_group_var = summary_state.update_group_var(group, "GOPR", 200.0)
if (summary_state.has_well_var("PROD1", "WGOR")):
	summary_state_well_var = summary_state.well_var("PROD1", "WGOR")
summary_state_elapsed = summary_state.elapsed()
summary_state_wells = summary_state.wells
# Get info from the EclipseState Object.
