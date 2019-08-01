#define BOOST_TEST_MODULE UDQ_Data

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well2.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <opm/output/eclipse/AggregateUDQData.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <opm/output/eclipse/InteHEAD.hpp>
#include <opm/output/eclipse/VectorItems/intehead.hpp>
#include <opm/output/eclipse/DoubHEAD.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQInput.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQParams.hpp>

//#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
//#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>

#include <opm/io/eclipse/OutputStream.hpp>

#include <stdexcept>
#include <utility>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace {
    
    Opm::Deck first_sim(std::string fname) {
        return Opm::Parser{}.parseFile(fname);        
    }

     Opm::UDQActive udq_active() {
      int update_count = 0;
      // construct record data for udq_active
      Opm::UDQParams params;
      Opm::UDQConfig conf(params);
      Opm::UDQActive udq_act;
      Opm::UDAValue uda1("WUOPRL");
      update_count += udq_act.update(conf, uda1, "PROD1", Opm::UDAControl::WCONPROD_ORAT);

      Opm::UDAValue uda2("WULPRL");
      update_count += udq_act.update(conf, uda2, "PROD1", Opm::UDAControl::WCONPROD_LRAT);
      Opm::UDAValue uda3("WUOPRU");
      update_count += udq_act.update(conf, uda3, "PROD2", Opm::UDAControl::WCONPROD_ORAT);
      Opm::UDAValue uda4("WULPRU");
      update_count += udq_act.update(conf, uda4, "PROD2", Opm::UDAControl::WCONPROD_LRAT);

      for (std::size_t index=0; index < udq_act.size(); index++)
      {
          const auto & record = udq_act[index];
          auto ind = record.input_index;
          auto udq_key = record.udq;
          auto name = record.wgname;
          auto ctrl_type = record.control;
       }
      return udq_act;
    }
}



//int main(int argc, char* argv[])
struct SimulationCase
{
    explicit SimulationCase(const Opm::Deck& deck)
        : es   { deck }
	, grid { deck }
	, sched{ deck, es }
    {}

    // Order requirement: 'es' must be declared/initialised before 'sched'.
    Opm::EclipseState es;
    Opm::EclipseGrid  grid;
    Opm::Schedule     sched;

};
    
BOOST_AUTO_TEST_SUITE(Aggregate_UDQ)


// test dimensions of multisegment data
BOOST_AUTO_TEST_CASE (Constructor)
{
    const auto simCase = SimulationCase{first_sim("UDQ_TEST_WCONPROD_IUAD-2.DATA")};
        
    Opm::EclipseState es = simCase.es;
    Opm::Schedule     sched = simCase.sched;
    Opm::EclipseGrid  grid = simCase.grid;
    const auto& ioConfig = es.getIOConfig();
    const auto& restart = es.cfg().restart();
    //Opm::UDQActive udq_act = udq_active();

    // Report Step 1: 2008-10-10 --> 2011-01-20
    const auto rptStep = std::size_t{1};    
    
    std::string outputDir = "./";
    std::string baseName = "TEST_UDQRST";
    Opm::EclIO::OutputStream::Restart rstFile {
    Opm::EclIO::OutputStream::ResultSet { outputDir, baseName },
    rptStep,
    Opm::EclIO::OutputStream::Formatted { ioConfig.getFMTOUT() },
	  Opm::EclIO::OutputStream::Unified   { ioConfig.getUNIFOUT() }
        };
	
    double secs_elapsed = 3.1536E07;
    const auto ih = Opm::RestartIO::Helpers::createInteHead(es, grid, sched,
                                                secs_elapsed, rptStep, rptStep);
       
    const auto udqDims = Opm::RestartIO::Helpers::createUdqDims(sched, rptStep, ih);
    auto  udqData = Opm::RestartIO::Helpers::AggregateUDQData(udqDims);
    udqData.captureDeclaredUDQData(sched, rptStep, ih);
     
    rstFile.write("IUDQ", udqData.getIUDQ());
    rstFile.write("IUAD", udqData.getIUAD());
    rstFile.write("ZUDN", udqData.getZUDN());
    rstFile.write("ZUDL", udqData.getZUDL());
    
    }
    
BOOST_AUTO_TEST_SUITE_END()
