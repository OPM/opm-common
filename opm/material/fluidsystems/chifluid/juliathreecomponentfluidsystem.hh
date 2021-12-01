#ifndef OPM_JULIATHREECOMPONENTFLUIDSYSTEM_HH
#define OPM_JULIATHREECOMPONENTFLUIDSYSTEM_HH

#include <opm/material/fluidsystems/BaseFluidSystem.hpp>

namespace Opm {
/*!
 * \ingroup FluidSystem
 *
 * \brief A two phase three component fluid system from the Julia test
 */

template <class Scalar>
class JuliaThreeComponentFluidSystem
: public Opm::BaseFluidSystem<Scalar, JuliaThreeComponentFluidSystem<Scalar> > {

}
#endif //OPM_JULIATHREECOMPONENTFLUIDSYSTEM_HH