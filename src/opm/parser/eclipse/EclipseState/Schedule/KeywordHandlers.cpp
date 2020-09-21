/*
  Copyright 2020 Statoil ASA.

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

#include <fnmatch.h>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <opm/common/OpmLog/LogUtil.hpp>
#include <opm/common/utility/numeric/cmp.hpp>
#include <opm/common/utility/String.hpp>

#include <opm/parser/eclipse/Python/Python.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckSection.hpp>
#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/L.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/N.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/V.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/W.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionX.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionResult.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicVector.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Events.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/SICD.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/Valve.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/WellSegments.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/OilVaporizationProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/RPTConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Tuning.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WList.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WListManager.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellFoamProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellInjectionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellPolymerProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellProductionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellBrineProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellConnections.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/Units/Dimension.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>

#include "Well/injection.hpp"

namespace Opm {

namespace {

}

    void Schedule::handleMESSAGES(const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        return applyMESSAGES(handlerContext.keyword, handlerContext.currentStep);
    }

    void Schedule::handleMULTFLT (const HandlerContext& handlerContext, const ParseContext&, ErrorGuard&) {
        this->m_modifierDeck[handlerContext.currentStep].addKeyword(handlerContext.keyword);
        m_events.addEvent( ScheduleEvents::GEO_MODIFIER, handlerContext.currentStep);
    }

    void Schedule::handleMXUNSUPP(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        std::string msg = "OPM does not support grid property modifier " + handlerContext.keyword.name() + " in the Schedule section. Error at report: " + std::to_string(handlerContext.currentStep);
        parseContext.handleError( ParseContext::UNSUPPORTED_SCHEDULE_GEO_MODIFIER , msg, errors );
    }

    void Schedule::handleWELOPEN (const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        return applyWELOPEN(handlerContext.keyword, handlerContext.currentStep, parseContext, errors);
    }


    bool Schedule::handleNormalKeyword(const HandlerContext& handlerContext, const ParseContext& parseContext, ErrorGuard& errors) {
        using handler_function = std::function<void(Schedule*, const HandlerContext&, const ParseContext&, ErrorGuard&)>;
        static const std::unordered_map<std::string,handler_function> handler_functions = {
            { "BRANPROP", &Schedule::handleBRANPROP },
            { "COMPDAT" , &Schedule::handleCOMPDAT  },
            { "COMPLUMP", &Schedule::handleCOMPLUMP },
            { "COMPORD" , &Schedule::handleCOMPORD  },
            { "COMPSEGS", &Schedule::handleCOMPSEGS },
            { "DRSDT"   , &Schedule::handleDRSDT    },
            { "DRSDTR"  , &Schedule::handleDRSDTR   },
            { "DRVDT"   , &Schedule::handleDRVDT    },
            { "DRVDTR"  , &Schedule::handleDRVDTR   },
            { "GCONINJE", &Schedule::handleGCONINJE },
            { "GCONPROD", &Schedule::handleGCONPROD },
            { "GCONSALE", &Schedule::handleGCONSALE },
            { "GCONSUMP", &Schedule::handleGCONSUMP },
            { "GEFAC"   , &Schedule::handleGEFAC    },
            { "GLIFTOPT", &Schedule::handleGLIFTOPT },
            { "GPMAINT" , &Schedule::handleGPMAINT  },
            { "GRUPNET" , &Schedule::handleGRUPNET  },
            { "GRUPTREE", &Schedule::handleGRUPTREE },
            { "GUIDERAT", &Schedule::handleGUIDERAT },
            { "LIFTOPT" , &Schedule::handleLIFTOPT  },
            { "LINCOM"  , &Schedule::handleLINCOM   },
            { "MESSAGES", &Schedule::handleMESSAGES },
            { "MULTFLT" , &Schedule::handleMULTFLT  },
            { "MULTPV"  , &Schedule::handleMXUNSUPP },
            { "MULTR"   , &Schedule::handleMXUNSUPP },
            { "MULTR-"  , &Schedule::handleMXUNSUPP },
            { "MULTREGT", &Schedule::handleMXUNSUPP },
            { "MULTSIG" , &Schedule::handleMXUNSUPP },
            { "MULTSIGV", &Schedule::handleMXUNSUPP },
            { "MULTTHT" , &Schedule::handleMXUNSUPP },
            { "MULTTHT-", &Schedule::handleMXUNSUPP },
            { "MULTX"   , &Schedule::handleMXUNSUPP },
            { "MULTX-"  , &Schedule::handleMXUNSUPP },
            { "MULTY"   , &Schedule::handleMXUNSUPP },
            { "MULTY-"  , &Schedule::handleMXUNSUPP },
            { "MULTZ"   , &Schedule::handleMXUNSUPP },
            { "MULTZ-"  , &Schedule::handleMXUNSUPP },
            { "NODEPROP", &Schedule::handleNODEPROP },
            { "NUPCOL"  , &Schedule::handleNUPCOL   },
            { "RPTSCHED", &Schedule::handleRPTSCHED },
            { "TUNING"  , &Schedule::handleTUNING   },
            { "UDQ"     , &Schedule::handleUDQ      },
            { "VAPPARS" , &Schedule::handleVAPPARS  },
            { "VFPINJ"  , &Schedule::handleVFPINJ   },
            { "VFPPROD" , &Schedule::handleVFPPROD  },
            { "WCONHIST", &Schedule::handleWCONHIST },
            { "WCONINJE", &Schedule::handleWCONINJE },
            { "WCONINJH", &Schedule::handleWCONINJH },
            { "WCONPROD", &Schedule::handleWCONPROD },
            { "WECON"   , &Schedule::handleWECON    },
            { "WEFAC"   , &Schedule::handleWEFAC    },
            { "WELOPEN" , &Schedule::handleWELOPEN  },
            { "WELSEGS" , &Schedule::handleWELSEGS  },
            { "WELSPECS", &Schedule::handleWELSPECS },
            { "WELTARG" , &Schedule::handleWELTARG  },
            { "WFOAM"   , &Schedule::handleWFOAM    },
            { "WGRUPCON", &Schedule::handleWGRUPCON },
            { "WHISTCTL", &Schedule::handleWHISTCTL },
            { "WINJTEMP", &Schedule::handleWINJTEMP },
            { "WLIFTOPT", &Schedule::handleWLIFTOPT },
            { "WLIST"   , &Schedule::handleWLIST    },
            { "WPIMULT" , &Schedule::handleWPIMULT  },
            { "WPMITAB" , &Schedule::handleWPMITAB  },
            { "WPOLYMER", &Schedule::handleWPOLYMER },
            { "WSALT"   , &Schedule::handleWSALT    },
            { "WSEGITER", &Schedule::handleWSEGITER },
            { "WSEGSICD", &Schedule::handleWSEGSICD },
            { "WSEGVALV", &Schedule::handleWSEGVALV },
            { "WSKPTAB" , &Schedule::handleWSKPTAB  },
            { "WSOLVENT", &Schedule::handleWSOLVENT },
            { "WTEMP"   , &Schedule::handleWTEMP    },
            { "WTEST"   , &Schedule::handleWTEST    },
            { "WTRACER" , &Schedule::handleWTRACER  },
        };

        const auto function_iterator = handler_functions.find(handlerContext.keyword.name());

        if (function_iterator != handler_functions.end()) {
            const auto& handler = function_iterator->second;

            handler(this, handlerContext, parseContext, errors);

            return true;
        } else {
            return false;
        }
    }

}
