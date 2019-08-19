#include <pybind11/pybind11.h>
#include "common.hpp"


PYBIND11_MODULE(libopmcommon_python, module) {
    opmcommon_python::export_Parser(module);
    opmcommon_python::export_Deck(module);
    opmcommon_python::export_DeckKeyword(module);
    opmcommon_python::export_Schedule(module);
    opmcommon_python::export_Well(module);
    opmcommon_python::export_Group(module);
    opmcommon_python::export_GroupTree(module);
    opmcommon_python::export_Connection(module);
    opmcommon_python::export_EclipseConfig(module);
    opmcommon_python::export_Eclipse3DProperties(module);
    opmcommon_python::export_EclipseState(module);
    opmcommon_python::export_TableManager(module);
    opmcommon_python::export_EclipseGrid(module);
}

