/*
  Copyright 2020 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef RST_STATE
#define RST_STATE

#include <memory>
#include <string>
#include <vector>

#include <opm/io/eclipse/rst/header.hpp>
#include <opm/io/eclipse/rst/aquifer.hpp>
#include <opm/io/eclipse/rst/group.hpp>
#include <opm/io/eclipse/rst/well.hpp>
#include <opm/io/eclipse/rst/udq.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/Tuning.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>

namespace Opm {
    class EclipseGrid;
} // namespace Opm

namespace Opm { namespace EclIO {
    class RestartFileView;
}} // namespace Opm::EclIO

namespace Opm { namespace RestartIO {

struct RstState {
    RstState(std::shared_ptr<EclIO::RestartFileView> rstView,
             const ::Opm::EclipseGrid*               grid);

    static RstState load(std::shared_ptr<EclIO::RestartFileView> rstView,
                         const Runspec& runspec,
                         const ::Opm::EclipseGrid*               grid = nullptr);

    const RstWell& get_well(const std::string& wname) const;

    ::Opm::UnitSystem unit_system;
    RstHeader header;
    RstAquifer aquifers;
    std::vector<RstWell> wells;
    std::vector<RstGroup> groups;
    std::vector<RstUDQ> udqs;
    RstUDQActive udq_active;
    Tuning tuning;

private:
    void load_tuning(const std::vector<int>& intehead,
                     const std::vector<double>& doubhead);

    void add_groups(const std::vector<std::string>& zgrp,
                    const std::vector<int>& igrp,
                    const std::vector<float>& sgrp,
                    const std::vector<double>& xgrp);

    void add_wells(const std::vector<std::string>& zwel,
                   const std::vector<int>& iwel,
                   const std::vector<float>& swel,
                   const std::vector<double>& xwel,
                   const std::vector<int>& icon,
                   const std::vector<float>& scon,
                   const std::vector<double>& xcon);

    void add_msw(const std::vector<std::string>& zwel,
                 const std::vector<int>& iwel,
                 const std::vector<float>& swel,
                 const std::vector<double>& xwel,
                 const std::vector<int>& icon,
                 const std::vector<float>& scon,
                 const std::vector<double>& xcon,
                 const std::vector<int>& iseg,
                 const std::vector<double>& rseg);

    void add_udqs(const std::vector<int>& iudq,
                  const std::vector<std::string>& zudn,
                  const std::vector<std::string>& zudl,
                  const std::vector<double>& dudw,
                  const std::vector<double>& dudg,
                  const std::vector<double>& dudf);

};

}} // namespace Opm::RestartIO

#endif
