/*
  Copyright (c) 2013-2015 Andreas Lauser
  Copyright (c) 2013 SINTEF ICT, Applied Mathematics.
  Copyright (c) 2013 Uni Research AS
  Copyright (c) 2015 IRIS AS

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
#include "config.h"

#include "EclipseWriter.hpp"

#include <opm/core/simulator/WellState.hpp>
#include <opm/output/eclipse/EclipseWriteRFTHandler.hpp>
#include <opm/common/ErrorMacros.hpp>
#include <opm/core/utility/Units.hpp>

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Units/ConversionFactors.hpp>
#include <opm/parser/eclipse/Units/Dimension.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <boost/algorithm/string/case_conv.hpp> // to_upper_copy
#include <boost/filesystem.hpp> // path

#include <memory>     // unique_ptr
#include <utility>    // move

#include <ert/ecl/fortio.h>
#include <ert/ecl/ecl_kw_magic.h>
#include <ert/ecl/ecl_init_file.h>
#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_rst_file.h>
#include <ert/ecl_well/well_const.h>
#include <ert/ecl/ecl_rsthead.h>
#define OPM_XWEL      "OPM_XWEL"

// namespace start here since we don't want the ERT headers in it
namespace Opm {
namespace EclipseWriterDetails {

// throw away the data for all non-active cells and reorder to the Cartesian logic of
// eclipse
void restrictAndReorderToActiveCells(std::vector<double> &data,
                                     int numCells,
                                     const int* compressedToCartesianCellIdx)
{
    if (!compressedToCartesianCellIdx)
        // if there is no active -> global mapping, all cells
        // are considered active
        return;

    std::vector<double> eclData;
    eclData.reserve( numCells );

    // activate those cells that are actually there
    for (int i = 0; i < numCells; ++i) {
        eclData.push_back( data[ compressedToCartesianCellIdx[i] ] );
    }
    data.swap( eclData );
}

// convert the units of an array
void convertFromSiTo(std::vector<double> &siValues, double toSiConversionFactor, double toSiOffset = 0)
{
    for (size_t curIdx = 0; curIdx < siValues.size(); ++curIdx) {
        siValues[curIdx] = unit::convert::to(siValues[curIdx] - toSiOffset, toSiConversionFactor);
    }
}


/**
 * Eclipse "keyword" (i.e. named data) for a vector.
 */
template <typename T>
class Keyword : private boost::noncopyable
{
public:
    // Default constructor
    Keyword()
        : ertHandle_(0)
    {}

    /// Initialization from double-precision array.
    Keyword(const std::string& name,
            const std::vector<double>& data)
        : ertHandle_(0)
    { set(name, data); }

    /// Initialization from double-precision array.
    Keyword(const std::string& name,
            const std::vector<int>& data)
        : ertHandle_(0)
    { set(name, data); }

    Keyword(const std::string& name,
            const std::vector<const char*>& data)
        : ertHandle_(0)
    {set(name, data); }

    ~Keyword()
    {
        if (ertHandle_)
            ecl_kw_free(ertHandle_);
    }

    template <class DataElementType>
    void set(const std::string name, const std::vector<DataElementType>& data)
    {

        if(ertHandle_) {
            ecl_kw_free(ertHandle_);
        }


        ertHandle_ = ecl_kw_alloc(name.c_str(),
                                  data.size(),
                                  ertType_());

        // number of elements to take
        const int numEntries = data.size();

        // fill it with values

        T* target = static_cast<T*>(ecl_kw_get_ptr(ertHandle()));
        for (int i = 0; i < numEntries; ++i) {
            target[i] = static_cast<T>(data[i]);
        }
    }

    void set(const std::string name, const std::vector<const char *>& data)
    {
      if(ertHandle_) {
          ecl_kw_free(ertHandle_);
      }


      ertHandle_ = ecl_kw_alloc(name.c_str(),
                                data.size(),
                                ertType_());

      // number of elements to take
      const int numEntries = data.size();
      for (int i = 0; i < numEntries; ++i) {
          ecl_kw_iset_char_ptr( ertHandle_, i, data[i]);
     }

    }

    ecl_kw_type *ertHandle() const
    { return ertHandle_; }

private:
    static ecl_type_enum ertType_()
    {
        if (std::is_same<T, float>::value)
        { return ECL_FLOAT_TYPE; }
        if (std::is_same<T, double>::value)
        { return ECL_DOUBLE_TYPE; }
        if (std::is_same<T, int>::value)
        { return ECL_INT_TYPE; }
        if (std::is_same<T, const char *>::value)
        { return ECL_CHAR_TYPE; }


        OPM_THROW(std::logic_error,
                  "Unhandled type for data elements in EclipseWriterDetails::Keyword");
    }

    ecl_kw_type *ertHandle_;
};

/**
 * Pointer to memory that holds the name to an Eclipse output file.
 */
class FileName : private boost::noncopyable
{
public:
    FileName(const std::string& outputDir,
             const std::string& baseName,
             ecl_file_enum type,
             int writeStepIdx,
             bool formatted)
    {
        ertHandle_ = ecl_util_alloc_filename(outputDir.c_str(),
                                             baseName.c_str(),
                                             type,
                                             formatted,
                                             writeStepIdx);
    }

    ~FileName()
    { std::free(ertHandle_); }

    const char *ertHandle() const
    { return ertHandle_; }

private:
    char *ertHandle_;
};

class Restart : private boost::noncopyable
{
public:
    static const int NIWELZ = 11; //Number of data elements per well in IWEL array in restart file
    static const int NZWELZ = 3;  //Number of 8-character words per well in ZWEL array restart file
    static const int NICONZ = 15; //Number of data elements per completion in ICON array restart file

    /**
     * The constants NIWELZ and NZWELZ referes to the number of elements per
     * well that we write to the IWEL and ZWEL eclipse restart file data
     * arrays. The constant NICONZ refers to the number of elements per
     * completion in the eclipse restart file ICON data array.These numbers are
     * written to the INTEHEAD header.

     * The elements are added in the method addRestartFileIwelData(...) and and
     * addRestartFileIconData(...), respectively.  We write as many elements
     * that we need to be able to view the restart file in Resinsight.  The
     * restart file will not be possible to restart from with Eclipse, we write
     * to little information to be able to do this.
     *
     * Observe that all of these values are our "current-best-guess" for how
     * many numbers are needed; there might very well be third party
     * applications out there which have a hard expectation for these values.
    */


    Restart(const std::string& outputDir,
            const std::string& baseName,
            int writeStepIdx,
            IOConfigConstPtr ioConfig)
    {

        ecl_file_enum type_of_restart_file = ioConfig->getUNIFOUT() ? ECL_UNIFIED_RESTART_FILE : ECL_RESTART_FILE;

        restartFileName_ = ecl_util_alloc_filename(outputDir.c_str(),
                                                   baseName.c_str(),
                                                   type_of_restart_file,
                                                   ioConfig->getFMTOUT(), // use formatted instead of binary output?
                                                   writeStepIdx);

        if ((writeStepIdx > 0) && (ECL_UNIFIED_RESTART_FILE == type_of_restart_file)) {
            restartFileHandle_ = ecl_rst_file_open_append(restartFileName_);
        }
        else {
            restartFileHandle_ = ecl_rst_file_open_write(restartFileName_);
        }
    }

    template <typename T>
    void add_kw(const Keyword<T>& kw)
    { ecl_rst_file_add_kw(restartFileHandle_, kw.ertHandle()); }


    void addRestartFileIwelData(std::vector<int>& iwel_data, size_t currentStep , WellConstPtr well, size_t offset) const {
        CompletionSetConstPtr completions = well->getCompletions( currentStep );

        iwel_data[ offset + IWEL_HEADI_ITEM ] = well->getHeadI() + 1;
        iwel_data[ offset + IWEL_HEADJ_ITEM ] = well->getHeadJ() + 1;
        iwel_data[ offset + IWEL_CONNECTIONS_ITEM ] = completions->size();
        iwel_data[ offset + IWEL_GROUP_ITEM ] = 1;

        {
            WellType welltype = well->isProducer(currentStep) ? PRODUCER : INJECTOR;
            int ert_welltype = EclipseWriter::eclipseWellTypeMask(welltype, well->getInjectionProperties(currentStep).injectorType);
            iwel_data[ offset + IWEL_TYPE_ITEM ] = ert_welltype;
        }

        iwel_data[ offset + IWEL_STATUS_ITEM ] = EclipseWriter::eclipseWellStatusMask(well->getStatus(currentStep));
    }

    void addRestartFileXwelData(const WellState& wellState, std::vector<double>& xwel_data) const {
        std::copy_n(wellState.bhp().begin(), wellState.bhp().size(), xwel_data.begin() + wellState.getRestartBhpOffset());
        std::copy_n(wellState.perfPress().begin(), wellState.perfPress().size(), xwel_data.begin() + wellState.getRestartPerfPressOffset());
        std::copy_n(wellState.perfRates().begin(), wellState.perfRates().size(), xwel_data.begin() + wellState.getRestartPerfRatesOffset());
        std::copy_n(wellState.temperature().begin(), wellState.temperature().size(), xwel_data.begin() + wellState.getRestartTemperatureOffset());
        std::copy_n(wellState.wellRates().begin(), wellState.wellRates().size(), xwel_data.begin() + wellState.getRestartWellRatesOffset());
    }

    void addRestartFileIconData(std::vector<int>& icon_data, CompletionSetConstPtr completions , size_t wellICONOffset) const {
        for (size_t i = 0; i < completions->size(); ++i) {
            CompletionConstPtr completion = completions->get(i);
            size_t iconOffset = wellICONOffset + i * Opm::EclipseWriterDetails::Restart::NICONZ;
            icon_data[ iconOffset + ICON_IC_ITEM] = 1;

            icon_data[ iconOffset + ICON_I_ITEM] = completion->getI() + 1;
            icon_data[ iconOffset + ICON_J_ITEM] = completion->getJ() + 1;
            icon_data[ iconOffset + ICON_K_ITEM] = completion->getK() + 1;

            {
                WellCompletion::StateEnum completion_state = completion->getState();
                if (completion_state == WellCompletion::StateEnum::OPEN) {
                    icon_data[ iconOffset + ICON_STATUS_ITEM ] = 1;
                } else {
                    icon_data[ iconOffset + ICON_STATUS_ITEM ] = 0;
                }
            }
            icon_data[ iconOffset + ICON_DIRECTION_ITEM] = (int)completion->getDirection();
        }
    }


    ~Restart()
    {
        free(restartFileName_);
        ecl_rst_file_close(restartFileHandle_);
    }

    void writeHeader( int writeStepIdx, ecl_rsthead_type* rsthead_data ){

      ecl_util_set_date_values( rsthead_data->sim_time , &rsthead_data->day , &rsthead_data->month , &rsthead_data->year );

      ecl_rst_file_fwrite_header(restartFileHandle_,
                                 writeStepIdx,
                                 rsthead_data);

    }

    ecl_rst_file_type *ertHandle() const
    { return restartFileHandle_; }

private:
    char *restartFileName_;
    ecl_rst_file_type *restartFileHandle_;
};

/**
 * The Solution class wraps the actions that must be done to the restart file while
 * writing solution variables; it is not a handle on its own.
 */
class Solution : private boost::noncopyable
{
public:
    Solution(Restart& restartHandle)
        : restartHandle_(&restartHandle)
    {  ecl_rst_file_start_solution(restartHandle_->ertHandle()); }

    ~Solution()
    { ecl_rst_file_end_solution(restartHandle_->ertHandle()); }

    template <typename T>
    void add(const Keyword<T>& kw)
    { ecl_rst_file_add_kw(restartHandle_->ertHandle(), kw.ertHandle()); }

    ecl_rst_file_type *ertHandle() const
    { return restartHandle_->ertHandle(); }

private:
    Restart* restartHandle_;
};

/**
 * Initialization file which contains static properties (such as
 * porosity and permeability) for the simulation field.
 */
class Init : private boost::noncopyable
{
public:
    Init(const std::string& outputDir,
         const std::string& baseName,
         int writeStepIdx,
         IOConfigConstPtr ioConfig) : egridFileName_(outputDir,
                                                     baseName,
                                                     ECL_EGRID_FILE,
                                                     writeStepIdx,
                                                     ioConfig->getFMTOUT())
    {
        bool formatted = ioConfig->getFMTOUT();

        FileName initFileName(outputDir,
                              baseName,
                              ECL_INIT_FILE,
                              writeStepIdx,
                              formatted);

        ertHandle_ = fortio_open_writer(initFileName.ertHandle(),
                                        formatted,
                                        ECL_ENDIAN_FLIP);
    }

    ~Init()
    { fortio_fclose(ertHandle_); }

    void writeHeader(int numCells,
                     const int* compressedToCartesianCellIdx,
                     time_t current_posix_time,
                     Opm::EclipseStateConstPtr eclipseState,
                     int ert_phase_mask,
                     const NNC& nnc = NNC())
    {
        auto dataField = eclipseState->get3DProperties().getDoubleGridProperty("PORO").getData();
        restrictAndReorderToActiveCells(dataField, numCells, compressedToCartesianCellIdx);

        auto eclGrid = eclipseState->getInputGridCopy();

        // update the ACTNUM array using the processed cornerpoint grid
        std::vector<int> actnumData(eclGrid->getCartesianSize(), 1);
        if (compressedToCartesianCellIdx) {
            std::fill(actnumData.begin(), actnumData.end(), 0);
            for (int cellIdx = 0; cellIdx < numCells; ++cellIdx) {
                int cartesianCellIdx = compressedToCartesianCellIdx[cellIdx];
                actnumData[cartesianCellIdx] = 1;
            }
        }

        eclGrid->resetACTNUM(&actnumData[0]);

        if (nnc.hasNNC())
        {
            int idx = 0;
            // const_cast is safe, since this is a copy of the input grid
            auto ecl_grid = const_cast<ecl_grid_type*>(eclGrid->c_ptr());
            for (NNCdata n : nnc.nncdata()) {
                ecl_grid_add_self_nnc( ecl_grid, n.cell1, n.cell2, idx++);
            }
        }


        // finally, write the grid to disk
        IOConfigConstPtr ioConfig = eclipseState->getIOConfigConst();
        if (ioConfig->getWriteEGRIDFile()) {
            if (eclipseState->getDeckUnitSystem().getType() == UnitSystem::UNIT_TYPE_METRIC){
                eclGrid->fwriteEGRID(egridFileName_.ertHandle(), true);
            }else{
                eclGrid->fwriteEGRID(egridFileName_.ertHandle(), false);
            }
        }


        if (ioConfig->getWriteINITFile()) {
            Keyword<float> poro_kw("PORO", dataField);
            ecl_init_file_fwrite_header(ertHandle(),
                                        eclGrid->c_ptr(),
                                        poro_kw.ertHandle(),
                                        ert_phase_mask,
                                        current_posix_time );
        }
    }

    void writeKeyword(const std::string& keywordName, const std::vector<double> &data)
    {
        Keyword <float> kw(keywordName, data);
        ecl_kw_fwrite(kw.ertHandle(), ertHandle());
    }

    fortio_type *ertHandle() const
    { return ertHandle_; }

private:
    fortio_type *ertHandle_;
    FileName egridFileName_;
};

} // end namespace EclipseWriterDetails



/**
 * Convert opm-core WellType and InjectorType to eclipse welltype
 */
int EclipseWriter::eclipseWellTypeMask(WellType wellType, WellInjector::TypeEnum injectorType)
{
  int ert_well_type = IWEL_UNDOCUMENTED_ZERO;

  if (PRODUCER == wellType) {
      ert_well_type = IWEL_PRODUCER;
  } else if (INJECTOR == wellType) {
      switch (injectorType) {
        case WellInjector::WATER:
          ert_well_type = IWEL_WATER_INJECTOR;
          break;
        case WellInjector::GAS:
          ert_well_type = IWEL_GAS_INJECTOR;
          break;
        case WellInjector::OIL :
          ert_well_type = IWEL_OIL_INJECTOR;
          break;
        default:
          ert_well_type = IWEL_UNDOCUMENTED_ZERO;
      }
  }

  return ert_well_type;
}


/**
 * Convert opm-core WellStatus to eclipse format: > 0 open, <= 0 shut
 */
int EclipseWriter::eclipseWellStatusMask(WellCommon::StatusEnum wellStatus)
{
  int well_status = 0;

  if (wellStatus == WellCommon::OPEN) {
    well_status = 1;
  }
  return well_status;
}



/**
 * Convert opm-core UnitType to eclipse format: ert_ecl_unit_enum
 */
ert_ecl_unit_enum
EclipseWriter::convertUnitTypeErtEclUnitEnum(UnitSystem::UnitType unit)
{
    switch (unit) {
      case UnitSystem::UNIT_TYPE_METRIC:
          return ERT_ECL_METRIC_UNITS;

      case UnitSystem::UNIT_TYPE_FIELD:
          return ERT_ECL_FIELD_UNITS;

      case UnitSystem::UNIT_TYPE_LAB:
          return ERT_ECL_LAB_UNITS;
    }

    throw std::invalid_argument("unhandled enum value");
}


void EclipseWriter::writeInit( time_t current_posix_time, double start_time, const NNC& nnc)
{
    // if we don't want to write anything, this method becomes a
    // no-op...
    if (!enableOutput_) {
        return;
    }

    writeStepIdx_ = 0;
    reportStepIdx_ = -1;

    EclipseWriterDetails::Init fortio(outputDir_, baseName_, /*stepIdx=*/0, eclipseState_->getIOConfigConst());
    fortio.writeHeader(numCells_,
                       compressedToCartesianCellIdx_,
                       current_posix_time,
                       eclipseState_,
                       ert_phase_mask_,
                       nnc);

    IOConfigConstPtr ioConfig = eclipseState_->getIOConfigConst();
    const auto& props = eclipseState_->get3DProperties();


    if (ioConfig->getWriteINITFile()) {
        if (props.hasDeckDoubleGridProperty("PERMX")) {
            auto data = props.getDoubleGridProperty("PERMX").getData();
            EclipseWriterDetails::convertFromSiTo(data, Opm::prefix::milli * Opm::unit::darcy);
            EclipseWriterDetails::restrictAndReorderToActiveCells(data, gridToEclipseIdx_.size(), gridToEclipseIdx_.data());
            fortio.writeKeyword("PERMX", data);
        }
        if (props.hasDeckDoubleGridProperty("PERMY")) {
            auto data = props.getDoubleGridProperty("PERMY").getData();
            EclipseWriterDetails::convertFromSiTo(data, Opm::prefix::milli * Opm::unit::darcy);
            EclipseWriterDetails::restrictAndReorderToActiveCells(data, gridToEclipseIdx_.size(), gridToEclipseIdx_.data());
            fortio.writeKeyword("PERMY", data);
        }
        if (props.hasDeckDoubleGridProperty("PERMZ")) {
            auto data = props.getDoubleGridProperty("PERMZ").getData();
            EclipseWriterDetails::convertFromSiTo(data, Opm::prefix::milli * Opm::unit::darcy);
            EclipseWriterDetails::restrictAndReorderToActiveCells(data, gridToEclipseIdx_.size(), gridToEclipseIdx_.data());
            fortio.writeKeyword("PERMZ", data);
        }
        if (nnc.hasNNC()) {
            std::vector<double> tran;
            for (NNCdata nd : nnc.nncdata()) {
                tran.push_back(nd.trans);
            }
            if (eclipseState_->getDeckUnitSystem().getType() == UnitSystem::UNIT_TYPE_METRIC)
                EclipseWriterDetails::convertFromSiTo(tran, 1.0 / Opm::Metric::Transmissibility);
            else
                EclipseWriterDetails::convertFromSiTo(tran, 1.0 / Opm::Field::Transmissibility);
            fortio.writeKeyword("TRANNNC", tran);
        }
    }
}

// implementation of the writeTimeStep method
void EclipseWriter::writeTimeStep(int report_step,
                                  time_t current_posix_time,
                                  double secs_elapsed,
                                  data::Solution cells,
                                  const WellState& wellState,
                                  bool  isSubstep)
{

    using dc = data::Solution::key;
    // if we don't want to write anything, this method becomes a
    // no-op...
    if (!enableOutput_) {
        return;
    }

    auto& pressure = cells[ dc::PRESSURE ];
    EclipseWriterDetails::convertFromSiTo(pressure, deckToSiPressure_);
    EclipseWriterDetails::restrictAndReorderToActiveCells(pressure, gridToEclipseIdx_.size(), gridToEclipseIdx_.data());

    if( cells.has( dc::SWAT ) ) {
        auto& saturation_water = cells[ dc::SWAT ];
        EclipseWriterDetails::restrictAndReorderToActiveCells(saturation_water, gridToEclipseIdx_.size(), gridToEclipseIdx_.data());
    }


    if( cells.has( dc::SGAS ) ) {
        auto& saturation_gas = cells[ dc::SGAS ];
        EclipseWriterDetails::restrictAndReorderToActiveCells(saturation_gas, gridToEclipseIdx_.size(), gridToEclipseIdx_.data());
    }

    IOConfigConstPtr ioConfig = eclipseState_->getIOConfigConst();


    // Write restart file
    if(!isSubstep && ioConfig->getWriteRestartFile(report_step))
    {
        const size_t ncwmax                 = eclipseState_->getSchedule()->getMaxNumCompletionsForWells(report_step);
        const size_t numWells               = eclipseState_->getSchedule()->numWells(report_step);
        std::vector<WellConstPtr> wells_ptr = eclipseState_->getSchedule()->getWells(report_step);

        std::vector<const char*> zwell_data( numWells * Opm::EclipseWriterDetails::Restart::NZWELZ , "");
        std::vector<int>         iwell_data( numWells * Opm::EclipseWriterDetails::Restart::NIWELZ , 0 );
        std::vector<int>         icon_data( numWells * ncwmax * Opm::EclipseWriterDetails::Restart::NICONZ , 0 );

        EclipseWriterDetails::Restart restartHandle(outputDir_, baseName_, report_step, ioConfig);

        const size_t sz = wellState.bhp().size() + wellState.perfPress().size() + wellState.perfRates().size() + wellState.temperature().size() + wellState.wellRates().size();
        std::vector<double>      xwell_data( sz , 0 );

        restartHandle.addRestartFileXwelData(wellState, xwell_data);

        for (size_t iwell = 0; iwell < wells_ptr.size(); ++iwell) {
            WellConstPtr well = wells_ptr[iwell];
            {
                size_t wellIwelOffset = Opm::EclipseWriterDetails::Restart::NIWELZ * iwell;
                restartHandle.addRestartFileIwelData(iwell_data, report_step, well , wellIwelOffset);
            }
            {
                size_t wellIconOffset = ncwmax * Opm::EclipseWriterDetails::Restart::NICONZ * iwell;
                restartHandle.addRestartFileIconData(icon_data,  well->getCompletions( report_step ), wellIconOffset);
            }
            zwell_data[ iwell * Opm::EclipseWriterDetails::Restart::NZWELZ ] = well->name().c_str();
        }


        {
            ecl_rsthead_type rsthead_data = {};
            rsthead_data.sim_time   = current_posix_time;
            rsthead_data.nactive    = numCells_;
            rsthead_data.nx         = cartesianSize_[0];
            rsthead_data.ny         = cartesianSize_[1];
            rsthead_data.nz         = cartesianSize_[2];
            rsthead_data.nwells     = numWells;
            rsthead_data.niwelz     = EclipseWriterDetails::Restart::NIWELZ;
            rsthead_data.nzwelz     = EclipseWriterDetails::Restart::NZWELZ;
            rsthead_data.niconz     = EclipseWriterDetails::Restart::NICONZ;
            rsthead_data.ncwmax     = ncwmax;
            rsthead_data.phase_sum  = ert_phase_mask_;
            rsthead_data.sim_days   = Opm::unit::convert::to(secs_elapsed, Opm::unit::day); //data for doubhead

            restartHandle.writeHeader( report_step, &rsthead_data);
        }


        restartHandle.add_kw(EclipseWriterDetails::Keyword<int>(IWEL_KW, iwell_data));
        restartHandle.add_kw(EclipseWriterDetails::Keyword<const char *>(ZWEL_KW, zwell_data));
        restartHandle.add_kw(EclipseWriterDetails::Keyword<double>(OPM_XWEL, xwell_data));
        restartHandle.add_kw(EclipseWriterDetails::Keyword<int>(ICON_KW, icon_data));


        EclipseWriterDetails::Solution sol(restartHandle);
        sol.add(EclipseWriterDetails::Keyword<float>("PRESSURE", pressure));


        // write the cell temperature
        auto& temperature = cells[ dc::TEMP ];
        EclipseWriterDetails::convertFromSiTo(temperature, deckToSiTemperatureFactor_, deckToSiTemperatureOffset_);
        sol.add(EclipseWriterDetails::Keyword<float>("TEMP", temperature));


        if( cells.has( dc::SWAT ) ) {
            sol.add( EclipseWriterDetails::Keyword<float>( "SWAT", cells[ dc::SWAT ] ) );
        }


        if( cells.has( dc::SGAS ) ) {
            sol.add( EclipseWriterDetails::Keyword<float>( "SGAS", cells[ dc::SGAS ] ) );
        }


        // Write RS - Dissolved GOR
        if( cells.has( dc::RS ) )
            sol.add(EclipseWriterDetails::Keyword<float>("RS", cells[ dc::RS ] ) );

        // Write RV - Volatilized oil/gas ratio
        if( cells.has( dc::RV ) )
            sol.add(EclipseWriterDetails::Keyword<float>("RV", cells[ dc::RV ] ) );
    }


    //Write RFT data for current timestep to RFT file
    std::shared_ptr<EclipseWriterDetails::EclipseWriteRFTHandler> eclipseWriteRFTHandler = std::make_shared<EclipseWriterDetails::EclipseWriteRFTHandler>(
                                                                                                                      compressedToCartesianCellIdx_,
                                                                                                                      numCells_,
                                                                                                                      eclipseState_->getInputGrid()->getCartesianSize());

    // Write RFT file.
    {
        char * rft_filename = ecl_util_alloc_filename(outputDir_.c_str(),
                                                      baseName_.c_str(),
                                                      ECL_RFT_FILE,
                                                      ioConfig->getFMTOUT(),
                                                      0);
        auto unit_type = eclipseState_->getDeckUnitSystem().getType();
        ert_ecl_unit_enum ecl_unit = convertUnitTypeErtEclUnitEnum(unit_type);
        std::vector<WellConstPtr> wells = eclipseState_->getSchedule()->getWells(report_step);
        eclipseWriteRFTHandler->writeTimeStep(*ioConfig,
                                              rft_filename,
                                              ecl_unit,
                                              report_step,
                                              current_posix_time,
                                              secs_elapsed,
                                              wells,
                                              eclipseState_->getInputGrid(),
                                              pressure,
                                              cells[ dc::SWAT ],
                                              cells[ dc::SGAS ] );
        free( rft_filename );
    }

    if (!isSubstep) {
        /* Summary variables (well reporting) */
    }

    ++writeStepIdx_;
    // store current report index
    reportStepIdx_ = report_step;
}


/// Convert OPM phase usage to ERT bitmask
static inline int ertPhaseMask( const TableManager& tm ) {
    return ( tm.hasPhase( Phase::PhaseEnum::WATER ) ? ECL_WATER_PHASE : 0 )
         | ( tm.hasPhase( Phase::PhaseEnum::OIL ) ? ECL_OIL_PHASE : 0 )
         | ( tm.hasPhase( Phase::PhaseEnum::GAS ) ? ECL_GAS_PHASE : 0 );
}

EclipseWriter::EclipseWriter(Opm::EclipseStateConstPtr eclipseState,
                             int numCells,
                             const int* compressedToCartesianCellIdx)
    : eclipseState_(eclipseState)
    , numCells_(numCells)
    , compressedToCartesianCellIdx_(compressedToCartesianCellIdx)
    , gridToEclipseIdx_(numCells, int(-1) )
    , ert_phase_mask_( ertPhaseMask( eclipseState->getTableManager() ) )
{
    const auto eclGrid = eclipseState->getInputGrid();
    cartesianSize_[0] = eclGrid->getNX();
    cartesianSize_[1] = eclGrid->getNY();
    cartesianSize_[2] = eclGrid->getNZ();

    if( compressedToCartesianCellIdx ) {
        // if compressedToCartesianCellIdx available then
        // compute mapping to eclipse order
        std::map< int , int > indexMap;
        for (int cellIdx = 0; cellIdx < numCells; ++cellIdx) {
            int cartesianCellIdx = compressedToCartesianCellIdx[cellIdx];
            indexMap[ cartesianCellIdx ] = cellIdx;
        }

        int idx = 0;
        for( auto it = indexMap.begin(), end = indexMap.end(); it != end; ++it ) {
            gridToEclipseIdx_[ idx++ ] = (*it).second;
        }
    }
    else {
        // if not compressedToCartesianCellIdx was given use identity
        for (int cellIdx = 0; cellIdx < numCells; ++cellIdx) {
            gridToEclipseIdx_[ cellIdx ] = cellIdx;
        }
    }

    // factor from the pressure values given in the deck to Pascals
    deckToSiPressure_ =
        eclipseState->getDeckUnitSystem().parse("Pressure")->getSIScaling();

    // factor and offset from the temperature values given in the deck to Kelvin
    deckToSiTemperatureFactor_ =
        eclipseState->getDeckUnitSystem().parse("Temperature")->getSIScaling();
    deckToSiTemperatureOffset_ =
        eclipseState->getDeckUnitSystem().parse("Temperature")->getSIOffset();

    init(eclipseState);
}

void EclipseWriter::init(Opm::EclipseStateConstPtr eclipseState)
{
    // get the base name from the name of the deck
    using boost::filesystem::path;
    auto ioConfig = eclipseState->getIOConfig();
    baseName_ = ioConfig->getBaseName();

    // make uppercase of everything (or otherwise we'll get uppercase
    // of some of the files (.SMSPEC, .UNSMRY) and not others
    baseName_ = boost::to_upper_copy(baseName_);

    // retrieve the value of the "output" parameter
    enableOutput_ = ioConfig->getOutputEnabled();

    // store in current directory if not explicitly set
    outputDir_ = ioConfig->getOutputDir();

    // set the index of the first time step written to 0...
    writeStepIdx_  = 0;
    reportStepIdx_ = -1;

    if (enableOutput_) {
        // make sure that the output directory exists, if not try to create it
        if (!boost::filesystem::exists(outputDir_)) {
            std::cout << "Trying to create directory \"" << outputDir_ << "\" for the simulation output\n";
            boost::filesystem::create_directories(outputDir_);
        }

        if (!boost::filesystem::is_directory(outputDir_)) {
            OPM_THROW(std::runtime_error,
                      "The path specified as output directory '" << outputDir_
                      << "' is not a directory");
        }
    }
}

} // namespace Opm
