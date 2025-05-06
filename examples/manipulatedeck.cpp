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

#include <opm/input/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/input/eclipse/EclipseState/IOConfig/IOConfig.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <opm/input/eclipse/Parser/InputErrorAction.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/A.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/I.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/Z.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/D.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/E.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/S.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>

#include <getopt.h>
#include <assert.h>
namespace fs = std::filesystem;

namespace {

Opm::Deck manipulate_deck(const char* deck_file, std::ostream& os)
{
  using namespace Opm;
    ParseContext parseContext(Opm::InputErrorAction::WARN);
    ErrorGuard errors;
    // Pad vertically
    auto deck = Opm::Parser{}.parseFile(deck_file, parseContext, errors);
    int nz_upper = 2; 
    double top_upper = 1500;
    bool no_gap= true;
    double min_dist = 0.0;
    double upper_poro= 0.1;
    //GridSection gridsec(decDeckSection gridsec(deck,"GRID");
    DeckSection gridsec(deck,"GRID");// fixing CARTDIMS? + ZCORN + double valued arrays.
    DeckSection runspec(deck,"RUNSPEC"); // fixing dimens
    DeckSection schedule(deck,"SCHEDULE");  // fixing COMPDAT
    //DeckSection props(deck,"PROPS"); // do nothing
    DeckSection regions(deck,"REGIONS"); // extend and add for equilnum
    DeckSection solution(deck,"SOLUTION"); // extend and add for equilnum
    
    ///DeckSection gridsec(deck,"EDIT");
    //std::vector<int>& cardims = deck.get<ParserKeywords::CARDIMS>().back().getSIDoubleData();
    //std::vector<int>& actnum = deck.get<ParserKeywords::ACTNUM>().back().getSIDoubleData();
    //std::vector<double>& coord = deck.get<ParserKeywords::COORD>().back().data();
    //std::vector<double>& zcorn = deck.get<ParserKeywords::ZCORN>().back().data();
    //const Opm::DeckView& actnum_view = gridsec.get<ParserKeywords::ACTNUM>();
    //Opm::DeckView& actnum_view = gridsec[](ParserKeywords::ACTNUM);

    std::vector<int>& actnum = const_cast<std::vector<int>&>(gridsec.get<ParserKeywords::ACTNUM>().back().getIntData());
    std::vector<value::status>& actnum_status = const_cast<std::vector<value::status>&>(gridsec.get<ParserKeywords::ACTNUM>().back().getValueStatus());
    //std::cout << runspec;
    
    int nx,ny,nz,nz_new,upper_equilnum;
    if(deck.hasKeyword<ParserKeywords::DIMENS>()){
        nx = deck[ParserKeywords::DIMENS::keywordName].back().getRecord(0).getItem("NX").get<int>(0);
        ny = deck[ParserKeywords::DIMENS::keywordName].back().getRecord(0).getItem("NY").get<int>(0);
        nz = deck[ParserKeywords::DIMENS::keywordName].back().getRecord(0).getItem("NZ").get<int>(0);
        nz_new= nz+nz_upper;
        {
            DeckItem& NZit = const_cast<DeckItem&>(deck[ParserKeywords::DIMENS::keywordName].back().getRecord(0).getItem("NZ"));
            std::vector<int>& data = NZit.getData<int>();
            data[0]=nz_new;
        }
        
        
        {
            DeckItem& eqldims = const_cast<DeckItem&>(deck[ParserKeywords::EQLDIMS::keywordName].back().getRecord(0).getItem("NTEQUL"));
            std::vector<int>& data = eqldims.getData<int>();
            data[0] += 1;
            upper_equilnum = data[0];
        }   
        //NZit.set<int>(0,nz+10);
    }else{
        std::cerr << "No DIMENS keyword found in the deck" << std::endl;
        return deck;
    }
    {
        DeckItem& NZit = const_cast<DeckItem&>(gridsec[ParserKeywords::SPECGRID::keywordName].back().getRecord(0).getItem("NZ"));
        std::vector<int>& data = NZit.getData<int>();
        data[0]=nz_new;
    }
  
    std::vector<double>& coord = const_cast<std::vector<double>&>(gridsec.get<ParserKeywords::COORD>().back().getRawDoubleData());
    std::vector<double>& zcorn = const_cast<std::vector<double>&>(gridsec.get<ParserKeywords::ZCORN>().back().getRawDoubleData());
    std::vector<value::status>& zcorn_status = const_cast<std::vector<value::status>&>(gridsec.get<ParserKeywords::ZCORN>().back().getValueStatus());
      //getRecord(0).getDataItem().getData< double >();
    //ZcornMapper mapper( getNX(), getNY(), getNZ());
    //zcorn_fixed = mapper.fixupZCORN( zcorn );
    //dimens[2] += 3;
    int nc_new = nx*ny*(nz+nz_upper);
    int nc = nx*ny*nz;
    //std::vector<DeckKeyword*> keywords = const_cast<std::vector<DeckKeyword*>>(gridsec.getKeywordList());
    for(const auto& records: gridsec){
        if(records.size() != 1){
            continue;
        }
        const DeckRecord& record = records.getRecord(0);
        if(record.size() != 1){
            continue;
        }
        const DeckItem& item = record.getItem(0);
        type_tag type_tag = item.getType();
        if(type_tag != type_tag::fdouble){
            continue;
        }
        std::vector<double>& val = const_cast<std::vector<double>&>(item.getData<double>());
        if(val.size() != nc){
            continue;
        }
        double default_value = 0.0;
        if(records.name() == std::string("PORO")){
            default_value = upper_poro;
        }

        std::cout << "Extend data_size: " << records.name() << std::endl;
        //std::cout << "Extend item name: " << item.name() << std::endl;
        std::vector<value::status>& val_status = const_cast<std::vector<value::status>&>(item.getValueStatus());

        std::vector<double> val_new(nc_new);
        std::vector<value::status> val_status_new(nc_new, value::status::deck_value);
        for(int i=0; i<nx;++i){
             for(int j=0; j<ny; ++j){
                 for(int k=0; k<nz_new; ++k){
                     int ind_new = i + nx*j + nx*ny*k;
                     int ind_old = i + nx*j + nx*ny*(k-nz_upper);
                     if(k>=nz_upper){
                         val_new[ind_new] = val[ind_old];
                         val_status_new[ind_new] = val_status[ind_old];
                     }else{
                         val_status_new[ind_new] = value::status::deck_value;
                         val_new[ind_new] = default_value;           
                     }   
                 }
             }
        }
        val_status = val_status_new;
        val = val_new;
    }
    // include all cells
    actnum.resize(nx*ny*nz_new);
    actnum_status = std::vector<value::status>(nx*ny*nz_new, value::status::deck_value);
    for(auto& act:actnum){
        act = 1;
    }
    // make pilars strait
    int ncoord = coord.size();
    int nxny= ncoord/6;
    assert((nx+1)*(ny+1) == nxny);
    assert(ncoord%6 == 0);
    for(int i=0; i<nxny; ++i){
        coord[6*i+3] = coord[6*i+0] ;
        coord[6*i+4] = coord[6*i+1] ;
        //coord[6*i+2] = 0;
        //coord[6*i+5] = coord[6*i+2];
    }
    std::vector<value::status> zcorn_status_new((2*nx)*(2*ny)*(2*nz_new),value::status::deck_value); 
    std::vector<double> zcorn_new((2*nx)*(2*ny)*(2*nz_new),0.0);
    // extend, make monotonic, fill gaps zcorn
    for (int i=0; i<2*nx; ++i){
        for(int j=0; j < 2*ny; ++j){
            double minz=1e20;
            double maxz=-1e20;
            for(int k=0; k< 2*nz; ++k){
                int index = i + j*(2*nx) + k*(2*nx)*(2*ny);
                double z = zcorn[index];
                minz = std::min(minz, z);
                maxz = std::max(maxz, z);
                //coord[6*index+5] = top_upper;
            }
            std::cout << "minz: " << minz << " maxz: " << maxz << std::endl;
            double dz_upper = (minz-top_upper)/nz_upper;
            std::cout << "dz_upper: " << dz_upper << std::endl;
            std::vector<double> zcornvert(2*nz_new);
            zcornvert[0] = top_upper;
            double z_prev = top_upper;
            for(int k=1; k<2*nz_new; k++){
                int k_old = k-2*nz_upper;
                //int ind_new = i + j*(2*nx) + k*(2*nx)*(2*ny);
                int ind_old = i + j*(2*nx) + k_old*(2*nx)*(2*ny);
                if(k_old>=0){
                        //double z_prev = zcorn[ind_prev];
                        double z = zcorn[ind_old];
                        double dz= z- z_prev;
                        //std::cout << dz << " " << z << " " << z_prev << std::endl;
                        if(dz<min_dist){   
                            dz=0;
                        }    
                        if(z==0.0){
                            dz=0;
                        }
                        if(no_gap){
                            // if we jup to new logical cell wee should not have gaps
                            if(k%2==0){
                                dz=0;
                            }
                        }
                        
                        zcornvert[k] = zcornvert[k-1]+dz;
                        z_prev = zcornvert[k];
                } else {
                    if(k%2==1){
                        zcornvert[k] = zcornvert[k-1]+dz_upper;
                    }else{
                        zcornvert[k] = zcornvert[k-1];
                    }
                    z_prev = zcornvert[k];
                }
            }
            for(int k=0; k<2*nz_new; k++){
                int ind = i + j*(2*nx) + k*(2*nx)*(2*ny);
                zcorn_new[ind] = zcornvert[k];
            }
               
        }
    }
    zcorn_status = zcorn_status_new;
    zcorn = zcorn_new;

    // fix kz for schedule i.e. COMPDAT
    for(const auto& records: schedule){
        //DeckView
        
        if( records.name() != std::string("COMPDAT")){
            std::cout << "Schedule record name not handled: " << records.name() << std::endl;
            continue;
        }
        std::cout << "Schedule record name: " << records.name() << std::endl;
        for(const auto& record: records){
            DeckRecord a;
            //std::cout << "Record: " << record << std::endl;
            int& K1 = const_cast<int&>(record.getItem("K1").getData<int>()[0]);
            int& K2 = const_cast<int&>(record.getItem("K2").getData<int>()[0]);
            K1 += nz_upper;
            K2 += nz_upper;
        //   for(const auto& item: record){
        //         std::cout << "Item name: " << item.name() << std::endl;
        //         auto type_tag = item.getType();
        //         //std::cout << "Item type: " << item.getType() << std::endl;
        //         //std::cout << "Item size: " << item.size() << std::endl;
        //         std::cout << "Item data size: " << item.data_size() << std::endl;
        //         //std::cout << "Item value status: " << item.getValueStatus() << std::endl;
        //     }
         }
    }

    for(const auto& records: deck){
        //DeckView
        
        if( (records.name() != std::string("EQUALS")) || 
            (records.name() != std::string("MULTIPLY")) ||
            (records.name() != std::string("COPY"))
        ){
            std::cout << "Schedule record name not handled: " << records.name() << std::endl;
            continue;
        }
        std::cout << "Schedule record name: " << records.name() << std::endl;
        for(const auto& record: records){
            DeckRecord a;
            //std::cout << "Record: " << record << std::endl;
            int& K1 = const_cast<int&>(record.getItem("K1").getData<int>()[0]);
            int& K2 = const_cast<int&>(record.getItem("K2").getData<int>()[0]);
            K1 += nz_upper;
            K2 += nz_upper;
        //   for(const auto& item: record){
        //         std::cout << "Item name: " << item.name() << std::endl;
        //         auto type_tag = item.getType();
        //         //std::cout << "Item type: " << item.getType() << std::endl;
        //         //std::cout << "Item size: " << item.size() << std::endl;
        //         std::cout << "Item data size: " << item.data_size() << std::endl;
        //         //std::cout << "Item value status: " << item.getValueStatus() << std::endl;
        //     }
         }
    }

    for(const auto& records: regions){
        if(records.size() != 1){
            continue;
        }
        const DeckRecord& record = records.getRecord(0);
        if(record.size() != 1){
            continue;
        }
        const DeckItem& item = record.getItem(0);
        type_tag type_tag = item.getType();
        if(type_tag != type_tag::integer){
            continue;
        }
        std::vector<int>& val = const_cast<std::vector<int>&>(item.getData<int>());
        if(val.size() != nc){
            continue;
        }
        int min_val = *std::min_element(val.begin(), val.end());
        int max_val = *std::max_element(val.begin(), val.end());
        int exteded_val = min_val;
        if(records.name() == std::string("EQUILNUM")){
            exteded_val = upper_equilnum;
        }
        //std::cout << "Extend data_size: " << .data_size() << std::endl;
        std::cout << "Extend item name: " << records.name() << std::endl;
        std::vector<value::status>& val_status = const_cast<std::vector<value::status>&>(item.getValueStatus());

        std::vector<int> val_new(nc_new);
        std::vector<value::status> val_status_new(nc_new, value::status::deck_value);
        for(int i=0; i<nx;++i){
             for(int j=0; j<ny; ++j){
                 for(int k=0; k<nz_new; ++k){
                     int ind_new = i + nx*j + nx*ny*k;
                     int ind_old = i + nx*j + nx*ny*(k-nz_upper);
                     if(k>=nz_upper){
                         val_new[ind_new] = val[ind_old];
                         val_status_new[ind_new] = val_status[ind_old];
                     }else{
                         val_status_new[ind_new] = value::status::deck_value;
                         val_new[ind_new] = exteded_val;           
                     }   
                 }
             }
        }
        val_status = val_status_new;
        val = val_new;
        
    }

    for(const auto& records: solution){
        if(records.name() == std::string("EQUIL")){
        
        // copy first record
        DeckKeyword& deckkeyword = const_cast<DeckKeyword&>(records);
        assert(records.size() >0);
        DeckRecord record = records.getRecord(0);
        //std::cout << "Solution record : " << record << std::endl;
        // for(const auto& item: record){
        //     std::cout << "Item name: " << item.name() << std::endl;
        //     auto type_tag = item.getType();
        //     //std::cout << "Item type: " << item.getType() << std::endl;
        //     //std::cout << "Item size: " << item.size() << std::endl;
        //     std::cout << "Item data size: " << item.data_size() << std::endl;
        //     //std::cout << "Item value status: " << item.getValueStatus() << std::endl;
        // }
        double& datum_pressure = const_cast<double&>(record.getItem("DATUM_PRESSURE").getData<double>()[0]);
        double& datum_depth = const_cast<double&>(record.getItem("DATUM_DEPTH").getData<double>()[0]);
        datum_pressure = 1.0;
        datum_depth = 0.0;
        double& woc = const_cast<double&>(record.getItem("OWC").getData<double>()[0]);
        double& goc = const_cast<double&>(record.getItem("GOC").getData<double>()[0]);
        woc = 0.0;
        goc = 0.0;
        deckkeyword.addRecord(std::move(record));
        }
        if(records.name() == std::string("RSVD")){
            DeckKeyword& deckkeyword = const_cast<DeckKeyword&>(records);
            assert(records.size() >0);
            DeckRecord record = records.getRecord(0);
            deckkeyword.addRecord(std::move(record));
        }

    }     
 


    os << deck;
    // int count=0;
    // assert(zcorn.size() == zcorn_new.size());
    // for(auto z : zcorn_new){
    //     std::cout << z << " ";
    //     count += 1;
    //     if(count%8 == 0){
    //         std::cout << std::endl;
    //     }
    // }
    //std::cout << count/8 << std::endl;
    return deck;
}

void print_help_and_exit()
{
    std::cerr << R"(
The manipulatedeck program will load a deck, resolve all include
files and then print it out again on stdout. All comments
will be stripped and the value types will be validated.

By passing the option -o you can redirect the output to a file
or a directory.

Print on stdout:

   manipulatedeck  /path/to/case/CASE.DATA


Print MY_CASE.DATA in /tmp:

    manipulatedeck -o /tmp /path/to/MY_CASE.DATA


Print NEW_CASE in cwd:

    opmpack -o NEW_CASE.DATA path/to/MY_CASE.DATA

As an alternative to the -o option you can use -c; that is equivalent to -o -
but restart and import files referred to in the deck are also copied. The -o and
-c options are mutually exclusive.
)";

    std::exit(EXIT_FAILURE);
}

void copy_file(const fs::path& source_dir, fs::path fname, const fs::path& target_dir)
{
    if (fname.is_absolute()) {
        // change when moving to gcc8+
        // fname = fs::relative(fname, source_dir);
        const auto prefix_len = fs::canonical(source_dir).string().size();
        fname = fs::canonical(fname);
        fname = fs::path(fname.string().substr(prefix_len + 1));
    }

    const auto source_file = source_dir / fname;
    const auto target_file = target_dir / fname;
    {
        const auto& parent_path = target_file.parent_path();
        if (!parent_path.empty() && !fs::is_directory(parent_path)) {
            fs::create_directories(parent_path);
        }
    }

    fs::copy_file(source_file, target_file, fs::copy_options::overwrite_existing);

    std::cerr << "Copying file " << source_file.string()
              << " -> " << target_file.string() << std::endl;
}

} // Anonymous namespace

int main(int argc, char** argv)
{
    int arg_offset = 1;
    bool stdout_output = true;
    bool copy_binary = false;
    const char* coutput_arg;

    while (true) {
        int c;
        c = getopt(argc, argv, "c:o:");
        if (c == -1)
            break;

        switch(c) {
        case 'o':
            stdout_output = false;
            coutput_arg = optarg;
            break;
        case 'c':
            stdout_output = false;
            copy_binary = true;
            coutput_arg = optarg;
            break;
        }
    }

    arg_offset = optind;
    if (arg_offset >= argc) {
        print_help_and_exit();
    }

    if (stdout_output) {
        manipulate_deck(argv[arg_offset], std::cout);
    }
    else {
        std::ofstream os;
        fs::path input_arg(argv[arg_offset]);
        fs::path output_arg(coutput_arg);
        fs::path output_dir(coutput_arg);

        if (fs::is_directory(output_arg)) {
            fs::path output_path = output_arg / input_arg.filename();
            os.open(output_path.string());
        }
        else {
            os.open(output_arg.string());
            output_dir = output_arg.parent_path();
        }

        const auto& deck = manipulate_deck(argv[arg_offset], os);
        if (copy_binary) {
            Opm::InitConfig init_config(deck);
            if (init_config.restartRequested()) {
                Opm::IOConfig io_config(deck);
                fs::path restart_file(io_config.getRestartFileName( init_config.getRestartRootName(), init_config.getRestartStep(), false ));
                copy_file(input_arg.parent_path(), restart_file, output_dir);
            }

            using IMPORT = Opm::ParserKeywords::IMPORT;
            for (const auto& import_keyword : deck.get<IMPORT>()) {
                const auto& fname = import_keyword.getRecord(0).getItem<IMPORT::FILE>().get<std::string>(0);
                copy_file(input_arg.parent_path(), fname, output_dir);
            }


            using PYACTION = Opm::ParserKeywords::PYACTION;
            for (const auto& pyaction_keyword : deck.get<PYACTION>()) {
                const auto& fname = pyaction_keyword.getRecord(1).getItem<PYACTION::FILENAME>().get<std::string>(0);
                copy_file(input_arg.parent_path(), fname, output_dir);
            }


            using GDFILE = Opm::ParserKeywords::GDFILE;
            if (deck.hasKeyword<GDFILE>()) {
                const auto& gdfile_keyword = deck.get<GDFILE>().back();
                const auto& fname = gdfile_keyword.getRecord(0).getItem<GDFILE::filename>().get<std::string>(0);
                copy_file(input_arg.parent_path(), fname, output_dir);
            }
        }
    }
}

