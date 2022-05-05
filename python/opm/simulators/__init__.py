# When using opm-simulators in combination with opm-common, the
#   opm-simulators pybind11 module is placed in a package opm2, i.e. opm2.simulators.
#   This is done to avoid conflict with the opm-common package prefix opm.
#   The following is hack to avoid having to write
#
#  from opm2.simulators import BlackOilSimulator
#  from opm.io.parser import Parser
#  #....
#
#   when it would be more natural to use the "opm" prefix instead of "opm2"
#   to import the BlackOilSimulator also. It assumes that PYTHONPATH includes
#   the install directory for the pybind11 python module for opm2.simulators

from opm2.simulators import BlackOilSimulator

