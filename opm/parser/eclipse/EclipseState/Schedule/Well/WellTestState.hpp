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
#ifndef WELLTEST_STATE_H
#define WELLTEST_STATE_H

#include <cstddef>
#include <string>
#include <vector>

#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellTestConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well.hpp>

namespace Opm {

namespace {

template<class BufferType, class T>
void pack_vector(BufferType& buffer, const std::vector<T>& v) {
    buffer.write(v.size());
    for (const auto& e : v)
        e.pack(buffer);
}

template<class BufferType, class T>
void unpack_vector(BufferType& buffer, std::vector<T>& v) {
    typename std::vector<T>::size_type size;
    buffer.read(size);
    for (std::size_t i = 0; i < size; i++) {
        T elm;
        elm.unpack(buffer);
        v.push_back(std::move(elm));
    }
}

}



class WellTestState {
public:
    /*
      This class implements a small mutable state object which keeps track of
      which wells have been automatically closed by the simulator, and by
      checking with the WTEST configuration object the class can return a list
      (well_name,reason) pairs for wells which should be checked as candiates
      for opening.
    */



    struct WTestWell {
        std::string name;
        WellTestConfig::Reason reason;
        // the well can be re-opened if the well testing is successful. We only test when it is closed.
        bool closed;
        // it can be the time of last test,
        // or the time that the well is closed if not test has not been performed after
        double last_test;
        int num_attempt;
        // if there is a WTEST setup for well testing,
        // it will be the report step that WTEST is specified.
        // if no, it is -1, which indicates we do not know the associated WTEST yet,
        // or there is not associated WTEST request
        int wtest_report_step;

        bool operator==(const WTestWell& other) const {
            return this->name == other.name &&
                   this->closed == other.closed &&
                   this->last_test == other.last_test &&
                   this->num_attempt == other.num_attempt &&
                   this->wtest_report_step == other.wtest_report_step;
        }

        template<class BufferType>
        void pack(BufferType& buffer) const {
            buffer.write(this->name);
            buffer.write(this->closed);
            buffer.write(this->last_test);
            buffer.write(this->num_attempt);
            buffer.write(this->wtest_report_step);
        }

        template<class BufferType>
        void unpack(BufferType& buffer) {
            buffer.read(this->name);
            buffer.read(this->closed);
            buffer.read(this->last_test);
            buffer.read(this->num_attempt);
            buffer.read(this->wtest_report_step);
        }
    };


    struct ClosedCompletion {
        std::string wellName;
        int complnum;
        double last_test;
        int num_attempt;

        bool operator==(const ClosedCompletion& other) const {
            return this->wellName == other.wellName &&
                   this->complnum == other.complnum &&
                   this->last_test == other.last_test &&
                   this->num_attempt == other.num_attempt;
        }

        template<class BufferType>
        void pack(BufferType& buffer) const {
            buffer.write(this->wellName);
            buffer.write(this->complnum);
            buffer.write(this->last_test);
            buffer.write(this->num_attempt);
        }

        template<class BufferType>
        void unpack(BufferType& buffer) {
            buffer.read(this->wellName);
            buffer.read(this->complnum);
            buffer.read(this->last_test);
            buffer.read(this->num_attempt);
        }
    };

    /*
      The simulator has decided to close a particular well; we then add it here
      as a closed well with a particualar reason.
    */
    void close_well(const std::string& well_name, WellTestConfig::Reason reason, double sim_time);

    /*
      The simulator has decided to close a particular completion in a well; we then add it here
      as a closed completions
    */
    void close_completion(const std::string& well_name, int complnum, double sim_time);

    /*
      The update will consult the WellTestConfig object and return a list of
      wells which should be checked for possible reopening; observe that the
      update method will update the internal state of the object by counting up
      the openiing attempts, and also set the time for the last attempt to open.
    */
    std::vector<std::string> updateWells(const WellTestConfig& config,
                                         const std::vector<Well>& wells_ecl,
                                         double sim_time);

    /*
      If the simulator decides that a constraint is no longer met the dropCompletion()
      method should be called to indicate that this reason for keeping the well
      closed is no longer active.
    */
    void dropCompletion(const std::string& well_name, int complnum);

    bool well_is_closed(const std::string& well_name, WellTestConfig::Reason reason) const;

    /* whether there is a well with the well_name closed in the WellTestState,
     * no matter what is the closing cause */
    bool well_is_closed(const std::string& well_name) const;

    void openWell(const std::string& well_name, WellTestConfig::Reason reason);

    /* open the well no matter what is the closing cause.
     * it is used when WELOPEN or WCON* keyword request to open the well */
    void openWell(const std::string& well_name);

    void open_completions(const std::string& well_name);

    bool completion_is_closed(const std::string& well_name, const int complnum) const;

    std::size_t num_closed_wells() const;
    std::size_t num_closed_completions() const;

    /*
      Return the last tested time for the well, or throw if no such well.
    */
    double lastTestTime(const std::string& well_name) const;

    void clear();

    template<class BufferType>
    void pack(BufferType& buffer) const {
        pack_vector(buffer, this->wells);
        pack_vector(buffer, this->completions);
    }

    template<class BufferType>
    void unpack(BufferType& buffer) {
        unpack_vector(buffer, this->wells);
        unpack_vector(buffer, this->completions);
    }

    bool operator==(const WellTestState& other) const;


private:
    std::vector<WTestWell> wells;
    std::vector<ClosedCompletion> completions;


    WTestWell* getWell(const std::string& well_name, WellTestConfig::Reason reason);

    void updateForNewWTEST(const WellTestConfig& config);
};


}

#endif

