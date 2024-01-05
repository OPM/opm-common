import os
import numpy as np
from opm.simulators import BlackOilSimulator
from opm.io.parser import Parser
from opm.io.ecl_state import EclipseState
from opm.io.schedule import Schedule
from opm.io.summary import SummaryConfig
from opm.io.deck import DeckKeyword

with open('name_grid.txt', 'r') as file:
    content = file.readlines()

result_lists = []
current_list = []

# A reveal name_grid.txt file contains grid information in the format of
# property name \n values \n / \n
for line in content:
    line = line.strip()
    current_list.append(line)
    
    if line == '/':
        concatenated_items = ' '.join(current_list).split()
        result_lists.append(concatenated_items)
        current_list = []
result_lists[:,0]
# results_lists is a list that contains lists of property names and values. Properties follow the order:
# ['DIMENSION', 'CORNERS', 'PORO', 'PERMX', 'MULTX', 'ROCKTYPE', 'SATNUM', 'IMBNUM', 'PVTNUM', 'EQLNUM', 'FIPNUM',
# 'SWINIT', 'SGINIT', 'POINIT', 'RSINIT', 'CGRINIT', 'TEMPINIT']

# Define grid size
dims = result_lists[0][1:-1] # The [1:-1] is to remove the / at the end of the list and the name at the start.
dims = [int(item) for item in dims] 
[nx, ny, nz] = dims

result_lists[1] = result_lists[1][1:-1]
result_lists[1] = np.array([float(item) for item in result_lists[1]])

# The reveal grid first includes the x values of the verices followed by the y values and then the z values.
# By default a reveal grid utilizes negative corner coordinates to signal inactive cells.
# We need to replace these negative values with the values of the closest active cells or else flow will crash.
result_lists[1] = result_lists[1].reshape(-1, 2*ny, 2*nx)

for l in range(result_lists[1].shape[0]):
    for j in range(2*ny):
        for i in range(2*nx):
            if result_lists[1][l][j][i] < 0:
                if all(result_lists[1][l][j][:i] < 0) and all(result_lists[1][l][j][i:] < 0):
                    continue
                else:
                    if i == 2*nx-1:
                        k = max(idx for idx, val in enumerate(result_lists[1][l][j][:i]) if val > 0 and result_lists[1][l][j][idx + 1] < 0)
                        values_to_assign = [result_lists[1][l][j][k] for x in range(1, i + 1 - k)]
                        result_lists[1][l][j][k + 1 : i + 1] = values_to_assign
                    elif result_lists[1][l][j][i+1] > 0:
                        if all(result_lists[1][l][j][:i] < 0):
                            values_to_assign = [result_lists[1][l][j][i+1] for x in range(1, i+2)]
                            result_lists[1][l][j][: i + 1] = values_to_assign[::-1]
                        else:
                            k = max(idx for idx, val in enumerate(result_lists[1][l][j][:i]) if val > 0 and result_lists[1][l][j][idx + 1] < 0)
                            values_to_assign = [result_lists[1][l][j][k] for x in range(1, i + 1 - k)]
                            result_lists[1][l][j][k+1: i + 1] = values_to_assign
                    else:
                        continue
            else:
                continue

    for i in range(2*nx):
        for j in range(2*ny):
            if result_lists[1][l][j][i] < 0:
                if all(result_lists[1][l][:j,i] < 0) and all(result_lists[1][l][j:,i] < 0):
                    continue
                else:
                    if j == 2*ny-1:
                        k = max(idx for idx, val in enumerate(result_lists[1][l][:j,i]) if val > 0 and result_lists[1][l][idx + 1,i] < 0)
                        values_to_assign = [result_lists[1][l][k,i] for x in range(1, j + 1 - k)]
                        result_lists[1][l][k+1:j+1,i ] = values_to_assign
                    elif result_lists[1][l][j+1,i] > 0:
                        if all(result_lists[1][l][:j,i] < 0):
                            values_to_assign = [result_lists[1][l][j+1,i] for x in range(1, j+2)]
                            result_lists[1][l][:j+1,i ] = values_to_assign[::-1]
                        else:
                            k = max(idx for idx, val in enumerate(result_lists[1][l][:j,i]) if val > 0 and result_lists[1][l][idx + 1,i] < 0)
                            values_to_assign = [result_lists[1][l][k,i] for x in range(1, j + 1 - k)]
                            result_lists[1][l][k+1:j+1,i] = values_to_assign
                    else:
                        continue
            else:
                continue

result_lists[1] = result_lists[1].flatten()

# zcorn data can be extracted straighforwardly since they have the same format that opm uses. 
zcorners = result_lists[1][2*8*nx*ny*nz:3*8*nx*ny*nz]

# We now need to define the values of the coord keyword. To do so we reshape result_lists[1] to a 2d array 
# where each column represents x, y, z coordinates respectively and then we generate coordinates.
coordinates = result_lists[1].reshape(3,-1).flatten('F').reshape(-1,3)

# We need to extract the coordinates of the first 4 vertices from the top layer and the last 4 vertices from the bottom layer. 
top_coords = coordinates[:4*nx*ny]
bottom_coords = coordinates[-4*nx*ny:]
top_coords =  top_coords.reshape(2*ny,2*nx,3)
bottom_coords =  bottom_coords.reshape(2*ny,2*nx,3)
coords = []
for j in range(2*ny):
    for i in range(2*nx):
        if (i % 2 == 0 or i % (2*nx-1) == 0) and (j % 2 == 0 or j % (2*ny-1) == 0):
            coords.append([top_coords[j,i,:], bottom_coords[j,i,:]])

coords = np.array(coords).flatten()

# Posority is zero for inactive cells. We can define actnum
# through porosity. If porosity is zero, actnum is zero.
poro = result_lists[2][1:-1]
poro = [float(item) for item in poro]
actnum = [0 if item == 0 else 1 for item in poro]

# We now extract the permeability values.
permx = result_lists[3][1:-1]
permx = [float(item) for item in permx]

# We now create the parser object and parse the runspec string.
parser = Parser()
Runspec_string = """ 
RUNSPEC
TITLE
RVL

OIL
GAS
CO2STORE
DISGAS
--THERMAL

FIELD

UNIFIN
UNIFOUT

DIMENS
	120 163 4 /

TABDIMS
 1 1 40 20 2 20 /

EQLDIMS
 1 1* 20 1 20 /


WELLDIMS
	 60 18 110 40 /



"""
deck = parser.parse_string(Runspec_string)
active_unit_system = deck.active_unit_system()
default_unit_system = deck.default_unit_system()

# After the first string is parsed, we can add new keywords manually with the deck.add keyword. 
# deck.add works only with activation and array keywords.
deck.add(DeckKeyword(parser['GRID']))
specGrid = DeckKeyword(parser['SPECGRID'], [[nx, ny, nz, 1, 'F']], active_unit_system, default_unit_system)
deck.add(specGrid)
coord = DeckKeyword(parser['COORD'], coords, active_unit_system, default_unit_system)
deck.add(coord)
zCorn = DeckKeyword(parser['ZCORN'], zcorners, active_unit_system, default_unit_system)
deck.add(zCorn)
actNum = DeckKeyword(parser['ACTNUM'], np.array(actnum))
deck.add(actNum)
poro = DeckKeyword(parser['PORO'], np.array(poro), active_unit_system, default_unit_system)
deck.add(poro)
permX = DeckKeyword(parser['PERMX'], np.array(permx), active_unit_system, default_unit_system)
deck.add(permX)
permy = permx
permY = DeckKeyword(parser['PERMY'], np.array(permy), active_unit_system, default_unit_system)
deck.add(permY)
permz = [0.2*item for item in permx]
permZ = DeckKeyword(parser['PERMZ'], np.array(permz), active_unit_system, default_unit_system)
deck.add(permZ)
deck.add(DeckKeyword(parser['PROPS']))

# Row and column vector keyword can't be added with deck.add method.
rock_string = """
ROCK
  1.0 1e-6 /

"""
deck = parser.parse_string(str(deck)+rock_string)

# Similarly column vector keywords can't be added with the deck.add method.
specrock_string = """
SPECROCK
 0 1.87
 100 1.87 /
"""
deck = parser.parse_string(str(deck)+specrock_string)
sgof_string = """
SGOF
0	0	1	0.0
1	1 	0 	0.0 /
"""
deck = parser.parse_string(str(deck)+sgof_string)

deck.add(DeckKeyword(parser['REGIONS']))
deck.add(DeckKeyword(parser['SOLUTION']))

solution_string = """  
EQUIL
 10000 4000 10000 0 0 0 1 1 0 /
    
RTEMPVD
  0 360
  100 360
/

RSVD
  0 0.0
  10000 0.0 /

RPTRST
 'BASIC=2' 'ALLPROPS'/
"""
deck = parser.parse_string(str(deck)+solution_string)

deck.add(DeckKeyword(parser['SCHEDULE']))
deck.add(DeckKeyword(parser['END']))

# We now write the deck to a file since I got a no input case error 
# when I tried to create the simulator object directly from the parsed string.
with open('REVEAL.DATA', 'w') as file:
    file.write(str(deck))

deck = parser.parse('REVEAL.DATA')
state = EclipseState(deck)
schedule = Schedule(deck, state)
summary_config = SummaryConfig(deck, state, schedule)

sim = BlackOilSimulator(deck, state, schedule, summary_config)
sim.step_init()
