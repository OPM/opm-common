#include <pybind11/pybind11.h>
#include "sunbeam.hpp"


PYBIND11_MODULE(libopmcommon_python, module) {
    sunbeam::export_Parser(module);
    sunbeam::export_Deck(module);
    sunbeam::export_DeckKeyword(module);
    sunbeam::export_Schedule(module);
    sunbeam::export_Well(module);
    sunbeam::export_Group(module);
    sunbeam::export_GroupTree(module);
    sunbeam::export_Connection(module);
    sunbeam::export_EclipseConfig(module);
    sunbeam::export_Eclipse3DProperties(module);
    sunbeam::export_EclipseState(module);
    sunbeam::export_TableManager(module);
    sunbeam::export_EclipseGrid(module);
}

