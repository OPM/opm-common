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

#include <iostream>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>  
#include <boost/filesystem.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/InputErrorAction.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp> 
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
namespace po = boost::program_options;
namespace pt = boost::property_tree;


#include <iostream>
#include <chrono>
typedef std::chrono::high_resolution_clock Clock;

inline void pack_deck( const char * deck_file, std::ostream& os, bool do_compare, bool do_ascii) {
    Opm::ParseContext parseContext(Opm::InputError::WARN);
    Opm::ErrorGuard errors;
    Opm::Parser parser;
    auto t1 = Clock::now();
    
    auto deck = parser.parseFile(deck_file, parseContext, errors);
    auto t2 = Clock::now();
    std::cout << "Parsing: "
	      << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
	      << " milliseconds" << std::endl;
    if(do_compare){
      t1 = Clock::now();
      os << deck;
      t2 = Clock::now();
      std::cout << "Stream writing: "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
		<< " milliseconds" << std::endl;   
    }
 
    if(do_ascii){
	{
	    t1 = Clock::now();
	    {  
		std::ofstream ofs("deck_serialized.ser");
		boost::archive::text_oarchive oa(ofs);
		oa << deck;
	  }
	    t2 = Clock::now();
	}

	std::cout << "Boost serializing ascii writing: "
		  << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
		  << " milliseconds" << std::endl;   
	
	std::cout << "Start deserialized deck "<< std::endl;
	{
	    Opm::Deck deck_new;
	    t1 = Clock::now();
	    {  
		std::ifstream ifs("deck_serialized.ser");
		boost::archive::text_iarchive ia(ifs);
		ia >> deck_new;
		std::cout << "dezerialized deck finnished "<< std::endl;
		
	    }
	    t2 = Clock::now();
	    std::cout << "Boost deserialising ascii reading: "
		      << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
		      << " milliseconds" << std::endl;   
	    
	    std::cout << "write out dezerialized deck" << std::endl;
	    deck_new.fullView();
    
	    std::ofstream file("deck_full_view.txt");
	    file << deck_new;
	}
    }
    {
      t1 = Clock::now();
      {  
	std::ofstream ofs("deck_serialized_bin.ser",std::ios::binary);
	boost::archive::binary_oarchive oa(ofs);
	oa << deck;
      }
      t2 = Clock::now();
      std::cout << "Boost serializing bin writing: "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
		<< " milliseconds" << std::endl;   
    }
    {
      Opm::Deck deck_new;
      t1 = Clock::now();
      {  
	std::ifstream ifs("deck_serialized_bin.ser", std::ios::binary);
	boost::archive::binary_iarchive ia(ifs);
	ia >> deck_new;
	std::cout << "dezerialized deck finnished "<< std::endl;     
      }
      t2 = Clock::now();
      std::cout << "Boost deserialising binary reading: "
		<< std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
		<< " milliseconds" << std::endl;
      if(do_compare){
	  std::ofstream file("deck_full_view_bin.txt",std::ios::binary);
	  deck_new.fullView();
	  file << deck_new;
      }
    }
 
}


void print_help_and_exit() {
    const char * help_text = R"(
The opmpack program will load a deck, resolve all include
files and then print it out again on stdout. All comments
will be stripped and the value types will be validated.

By passing the option -o you can redirect the output to a file
or a directory.

Print on stdout:

   opmpack  /path/to/case/CASE.DATA


Print MY_CASE.DATA in /tmp:

    opmpack -o /tmp /path/to/MY_CASE.DATA


Print NEW_CASE in cwd:

    opmpack -o NEW_CASE.DATA path/to/MY_CASE.DATA


)";
    std::cerr << help_text << std::endl;
    exit(1);
}


int main(int argc, char** argv) {
    po::variables_map vm;
    bool stdout_output = false;
    try {
        po::options_description desc("Allowed options");
        desc.add_options()
	    ("help", "produce help message")
	    ("f", po::value<std::string>(),"input file")
	    ("o", po::value<std::string>(), "output file")
	    ("a", po::value<bool>()->default_value(false), "Do ascii")
	    ("c", po::value<bool>()->default_value(false), "Compare");
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	if (vm.count("help")) {
            std::cout << desc << "\n";
            return 1;
        }

        if (!vm.count("f")) {
            std::cout << "f: inpufilenot set.\n";
            return 1;
        }
        else if (!vm.count("o")) {
            std::cout << "o outputfile not set.\n";
	    stdout_output = true;
        }
    } catch (std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Exception of unknown type!\n";
    }

    bool do_ascii = vm["a"].as<bool>();
    bool do_compare = vm["c"].as<bool>();
    std::string inputfile = vm["f"].as<std::string>();
    std::cout << "Inputfile : " << inputfile << std::endl;
    if (stdout_output)
        pack_deck(inputfile.c_str(), std::cout, do_compare, do_ascii);
    else {
        std::ofstream os;
        using path = boost::filesystem::path;
	std::string outputfile = vm["o"].as<std::string>();
	std::cout << "Outputfile : " << outputfile << std::endl;
        path input_arg(inputfile.c_str());
        path output_arg(outputfile.c_str());
        if (boost::filesystem::is_directory(output_arg)) {
            path output_path = output_arg / input_arg.filename();
            os.open(output_path.string());
        } else
            os.open(output_arg.string());

        pack_deck(inputfile.c_str(), os, do_compare, do_ascii);
    }
}

