/*
  Copyright 2018 Statoil ASA.

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


#ifndef UDQState_HPP_
#define UDQState_HPP_

#include <opm/parser/eclipse/EclipseState/Schedule/UDQExpression.hpp>


namespace Opm {

    class UDQ{
    public:
        explicit UDQ(const Deck& deck);
        const std::vector<UDQExpression>& expressions() const noexcept;
    private:
        std::vector<UDQExpression> m_expressions;
    };
}



#endif
