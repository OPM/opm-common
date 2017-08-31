#ifndef ECLIPSE_STATE_HPP
#define ECLIPSE_STATE_HPP

#include <boost/python.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaceDir.hpp>


namespace Opm {
    class EclipseState;
}

namespace eclipse_state {
    namespace py = boost::python;
    using namespace Opm;

    py::list getNNC( const EclipseState& state );
    py::list faultNames( const EclipseState& state );
    py::dict jfunc( const EclipseState& s);
    const std::string faceDir( FaceDir::DirEnum dir );
    py::list faultFaces( const EclipseState& state, const std::string& name );

    void export_EclipseState();

}

#endif //ECLIPSE_STATE_HPP
