/*
  Copyright 2015 Statoil ASA.

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

#ifndef OPM_IO_CONFIG_HPP
#define OPM_IO_CONFIG_HPP

#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>


namespace Opm {

    class IOConfig {

    public:

        explicit IOConfig(const std::string& input_path = "");


        bool getWriteRestartFile(size_t timestep) const;
        bool getWriteEGRIDFile() const;
        bool getWriteINITFile() const;
        bool getUNIFOUT() const;
        bool getUNIFIN() const;
        bool getFMTIN() const;
        bool getFMTOUT() const;
        const std::string& getEclipseInputPath() const;

        void overrideRestartWriteInterval(size_t interval);

        void handleRPTRSTBasic(TimeMapConstPtr timemap,
                               size_t timestep,
                               size_t basic,
                               size_t frequency = 1,
                               bool update_default = false,
                               bool reset_global = false);
        void handleRPTSCHEDRestart(TimeMapConstPtr timemap, size_t timestep, size_t restart);
        void handleSolutionSection(TimeMapConstPtr timemap, std::shared_ptr<const SOLUTIONSection> solutionSection);
        void handleGridSection(std::shared_ptr<const GRIDSection> gridSection);
        void handleRunspecSection(std::shared_ptr<const RUNSPECSection> runspecSection);
        void dumpRestartConfig() const;
    private:






        void initRestartOutputConfig(TimeMapConstPtr timemap);
        bool getWriteRestartFileFrequency(size_t timestep,
                                          size_t start_index,
                                          size_t frequency,
                                          bool first_timesteps_years  = false,
                                          bool first_timesteps_months = false) const;


        TimeMapConstPtr m_timemap;
        bool            m_write_INIT_file;
        bool            m_write_EGRID_file;
        bool            m_UNIFIN;
        bool            m_UNIFOUT;
        bool            m_FMTIN;
        bool            m_FMTOUT;
        std::string     m_eclipse_input_path;
        bool            m_ignore_RPTSCHED_RESTART;


        struct restartConfig {
            size_t timestep;
            size_t basic;
            size_t frequency;
            bool   rptsched_restart_set;
            size_t rptsched_restart;

            bool operator!=(const restartConfig& rhs) {
                bool ret = true;
                if ((this->timestep  == rhs.timestep) &&
                    (this->basic     == rhs.basic) &&
                    (this->frequency == rhs.frequency)) {
                        ret = false;
                }
                return ret;
            }
        };


        std::shared_ptr<DynamicState<restartConfig>> m_restart_output_config;

    };


    typedef std::shared_ptr<IOConfig> IOConfigPtr;
    typedef std::shared_ptr<const IOConfig> IOConfigConstPtr;

} //namespace Opm



#endif
