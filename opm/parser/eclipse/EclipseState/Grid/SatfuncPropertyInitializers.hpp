/*
  Copyright 2014 Andreas Lauser

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef ECLIPSE_SATFUNCPROPERTY_INITIALIZERS_HPP
#define ECLIPSE_SATFUNCPROPERTY_INITIALIZERS_HPP

#include <vector>
#include <string>

namespace Opm {

    class Deck;
    class EclipseState;

    std::vector< double >& SGLEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& ISGLEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& SGUEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& ISGUEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& SWLEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& ISWLEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& SWUEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& ISWUEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& SGCREndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& ISGCREndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& SOWCREndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& ISOWCREndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& SOGCREndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& ISOGCREndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& SWCREndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& ISWCREndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& PCWEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& IPCWEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& PCGEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& IPCGEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& KRWEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& IKRWEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& KRWREndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& IKRWREndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& KROEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& IKROEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& KRORWEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& IKRORWEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& KRORGEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& IKRORGEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& KRGEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& IKRGEndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& KRGREndpoint( std::vector< double >&, const Deck&, const EclipseState& );
    std::vector< double >& IKRGREndpoint( std::vector< double >&, const Deck&, const EclipseState& );

}

#endif
