/*
  Copyright 2018 Statoil ASA

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

#include <opm/output/eclipse/AggregateGroupData.hpp>
#include <opm/output/eclipse/AggregateNetworkData.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>
#include <opm/output/eclipse/VectorItems/group.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>
#include <opm/output/eclipse/VectorItems/intehead.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GTNode.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Network/ExtNetwork.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Network/Branch.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Network/Node.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>


#include <algorithm>
#include <cstddef>
#include <cstring>
#include <exception>
#include <string>
#include <stdexcept>

#define ENABLE_GCNTL_DEBUG_OUTPUT 0

#if ENABLE_GCNTL_DEBUG_OUTPUT
#include <iostream>
#endif // ENABLE_GCNTL_DEBUG_OUTPUT

// #####################################################################
// Class Opm::RestartIO::Helpers::AggregateNetworkData
// ---------------------------------------------------------------------


namespace {

// maximum number of groups
std::size_t ngmaxz(const std::vector<int>& inteHead)
{
    return inteHead[Opm::RestartIO::Helpers::VectorItems::NGMAXZ];
}

// maximum number of network nodes
std::size_t nodmax(const std::vector<int>& inteHead)
{
    return inteHead[Opm::RestartIO::Helpers::VectorItems::NODMAX];
}

// maximum number of network branches
std::size_t nbrmax(const std::vector<int>& inteHead)
{
    return inteHead[Opm::RestartIO::Helpers::VectorItems::NBRMAX];
}

std::size_t entriesPerInobr(const std::vector<int>& inteHead)
{
    return inteHead[Opm::RestartIO::Helpers::VectorItems::NINOBR];
}

template <typename NodeOp>
void nodeLoop(const std::vector<const std::string*>& nodeNamePtrs,
              NodeOp&&                             nodeOp)
{
    auto nodeID = std::size_t {0};
    for (const auto* nodeNmPtr : nodeNamePtrs) {
        nodeID += 1;

        if (nodeNmPtr == nullptr) {
            continue;
        }

        nodeOp(*nodeNmPtr, nodeID - 1);
    }
}

template <typename BranchOp>
void branchLoop(const std::vector<const Opm::Network::Branch*>& branches,
                BranchOp&&                             branchOp)
{
    auto branchID = std::size_t {0};
    for (const auto* branch : branches) {
        branchID += 1;

        if (branch   == nullptr) {
            continue;
        }

        branchOp(*branch, branchID - 1);
    }
}

template < typename T>
std::pair<bool, int > findInVector(const std::vector<T>  & vecOfElements, const T  & element)
{
    std::pair<bool, int > result;

    // Find given element in vector
    auto it = std::find(vecOfElements.begin(), vecOfElements.end(), element);

    if (it != vecOfElements.end())
    {
        result.second = std::distance(vecOfElements.begin(), it);
        result.first = true;
    }
    else
    {
        result.first = false;
        result.second = -1;
    }
    return result;
}

int next_branch(int node_no, std::vector<int> inlets, std::vector<int> outlets)
{
    auto res_inlets = findInVector<int>(inlets, node_no);
    auto res_outlets = findInVector<int>(outlets, node_no);

    if ((!res_inlets.first) && (!res_outlets.first)) {
        return 0;
    }

    if (res_outlets.first) {
        return res_outlets.second + 1;
    } else if (res_inlets.first) {
        return (res_inlets.second + 1) * (-1);
    }
}


std::vector<int> inobrFunc( const Opm::Schedule&    sched,
                            const std::size_t                 lookup_step,
                            const std::vector<int>&           inteHead)
{
    const auto& niobr = inteHead[Opm::RestartIO::Helpers::VectorItems::intehead::NINOBR];
    const auto& ntwNdNm = sched[lookup_step].network().insert_index_nd_names();
    const auto& branchPtrs = sched[lookup_step].network().branches();

    std::vector<int> newInobr;
    const int used_flag = -9;
    std::vector<int> inlets;
    std::vector<int> outlets;
    int ind;

    for (const auto& branch : branchPtrs) {
        auto dwntr_nd_res = findInVector<std::string>(ntwNdNm, branch->downtree_node());
        ind = (dwntr_nd_res.first) ? dwntr_nd_res.second + 1 : 0 ;
        inlets.push_back(ind);
        auto uptr_nd_res = findInVector<std::string>(ntwNdNm, branch->uptree_node());
        ind = (dwntr_nd_res.first) ? uptr_nd_res.second + 1 : 0 ;
        outlets.push_back(ind);
    }

    int n1 = inlets[0];
    int ind_br = 0;
    newInobr.push_back(n1 * (-1));
    inlets[0] = used_flag;

    while (n1 <= ntwNdNm.size()) {
        ind_br = next_branch(n1, inlets, outlets);
        while (ind_br != 0) {
            newInobr.push_back(ind_br);
            if (ind_br > 0) {
                outlets[ind_br-1] = used_flag;
            } else {
                inlets[ind_br*(-1) - 1] = used_flag;
            }
            ind_br = next_branch(n1, inlets, outlets);
        }
        n1 += 1;
    }
    return newInobr;
}

bool fixedPressureNode(const Opm::Schedule&         sched,
                       const std::string&           nodeName,
                       const size_t                 lookup_step
                      )
{
    auto& network = sched[lookup_step].network();
    bool fpn = (network.node(nodeName).terminal_pressure().has_value()) ? true  : false;
    return fpn;
}

double nodePressure(const Opm::Schedule&               sched,
                    const ::Opm::SummaryState&         smry,
                    const std::string&                 nodeName,
                    const Opm::UnitSystem&             units,
                    const size_t                       lookup_step
                   )
{
    using M = ::Opm::UnitSystem::measure;
    double node_pres = 1.;
    bool node_wgroup = false;
    const auto& wells = sched.getWells(lookup_step);
    auto& network = sched[lookup_step].network();

    // If a node is a well group, set the node pressure to the well's thp-limit if this is larger than the default value (1.)
    for (const auto& well : wells) {
        const auto& wgroup_name = well.groupName();
        if (wgroup_name == nodeName) {
            if (well.isProducer()) {
                const auto& pc = well.productionControls(smry);
                const auto& predMode = well.predictionMode();
                if (pc.thp_limit >= node_pres) {
                    node_pres = units.from_si(M::pressure, pc.thp_limit);
                    node_wgroup = true;
                }
            }
        }
    }

    // for nodes that are not well groups, set the node pressure to the fixed pressure potentially higher in the node tree
    if (!node_wgroup) {
        if (fixedPressureNode(sched, nodeName, lookup_step)) {
            // node is a fixed pressure node
            node_pres = units.from_si(M::pressure, network.node(nodeName).terminal_pressure().value());
        }
        else {
            // find fixed pressure higher in the node tree
            bool fp_flag = false;
            auto node_name = nodeName;
            auto upt_br = network.uptree_branch(node_name).value();
            while (!fp_flag) {
                if (fixedPressureNode(sched, upt_br.uptree_node(), lookup_step)) {
                    node_pres = units.from_si(M::pressure, network.node(upt_br.uptree_node()).terminal_pressure().value());
                    fp_flag = true;
                } else {
                    node_name = upt_br.uptree_node();
                    if (network.uptree_branch(node_name).has_value()) {
                        upt_br = network.uptree_branch(node_name).value();
                    } else {
                        std::stringstream str;
                        str << "Node: " << nodeName << " has no uptree node with fixe pressure condition, uppermost node:  " << node_name;
                        throw std::invalid_argument(str.str());
                    }
                }
            }
        }
    }
    return node_pres;
}

struct branchDenVec
{
    std::vector<double> brDeno;
    std::vector<double> brDeng;
};


struct nodeProps
{
    double ndDeno;
    double ndDeng;
    double ndOpr;
    double ndGpr;
};


nodeProps nodeRateDensity(const Opm::EclipseState&                  es,
                            const Opm::Schedule&                      sched,
                            const ::Opm::SummaryState&                smry,
                            const std::string&                        nodeName,
                            const Opm::UnitSystem&                    units,
                            const size_t                              lookup_step
                            )
{
    using M = ::Opm::UnitSystem::measure;

    auto get = [&smry](const ::Opm::Well& well, const std::string& vector)
    {
        const auto key = vector + ':' + well;

        return smry.has(key) ? smry.get(key) : 0.0;
    };

    const auto& network = sched[lookup_step].network();
    const auto& stdDensityTable = es.getTableManager().getDensityTable();
    //const auto& branchPtrs = sched[lookup_step].network().branches();
    //bool node_wgroup = false;
    const auto& wells = sched.getWells(lookup_step);
    //const auto& curGroups = sched.restart_groups(lookup_step);

    std::vector<nodeProps> nd_prop_vec;
    nodeProps nd_prop;

    double deno = 0.;
    double deng = 0.;
    double opr  = 0.;
    double gpr  = 0.;
    double t_opr = 0.;
    double t_gpr = 0.;


    //const auto& nodeName = branch.downtree_node();
    std::string node_nm = nodeName;
    // loop over downtree branches
    for (const auto& br : network.downtree_branches(nodeName)) {
        node_nm = br.downtree_node();
        // check if node is a group
        if (sched.hasGroup(node_nm)) {
            // check if group is a well group
            const auto& grp = sched.getGroup(node_nm, lookup_step);
            if (grp.wellgroup()) {
                // Calculate average flow rate and surface condition densities for wells in group
                for (const auto& well : wells) {
                    const auto& wgroup_name = well.groupName();
                    if (wgroup_name == node_nm) {
                        if (well.isProducer()) {
                            const auto& pc = well.productionControls(smry);
                            const auto& pvtNum = well.pvt_table;
                            t_opr = get(well,"WOPR");
                            deno += t_opr * stdDensityTable[pvtNum].oil;
                            opr  += t_opr;
                            t_gpr = get(well,"WGPR");
                            deng += t_gpr * stdDensityTable[pvtNum].gas;
                            gpr  += t_gpr;
                        }
                    }
                }
                deno = deno/opr;
                deng = deng/gpr;
                nd_prop.ndDeno = deno;
                nd_prop.ndOpr  = opr;
                nd_prop.ndDeng = deng;
                nd_prop.ndGpr  = gpr;
                nd_prop_vec.push_back(nd_prop);
            }
        } else {
            // Node group - step one level down in the branch tree and repeat ...
            nd_prop = nodeRateDensity(es, sched, smry, node_nm, units, lookup_step);
            nd_prop_vec.push_back(nd_prop);
        }
    }
    // calculate the total rates and average densities and return object
    deno = 0.;
    deng = 0.;
    opr  = 0.;
    gpr  = 0.;
    for (const auto& ndp : nd_prop_vec) {
        opr += ndp.ndOpr;
        gpr += ndp.ndGpr;
        deno += ndp.ndDeno*ndp.ndOpr;
        deng += ndp.ndDeng*ndp.ndGpr;
    }
    deno = deno/opr;
    deng = deng/gpr;

    return {deno, deng, opr, gpr};
}

branchDenVec branchesDensity(const Opm::Schedule&                    sched,
                             const ::Opm::SummaryState&                  smry,
                             const ::Opm::Network::Branch&               branch,
                             const Opm::UnitSystem&                      units,
                             const size_t                                lookup_step
                            )
{
    using M = ::Opm::UnitSystem::measure;
    const auto& branchPtrs = sched[lookup_step].network().branches();
    bool node_wgroup = false;
    const auto& wells = sched.getWells(lookup_step);
    auto& network = sched[lookup_step].network();

    const auto& nodeName = branch.downtree_node();
    // loop over all downtree branches
    for (const auto

            // If a node is a well group, set the node pressure to the well's thp-limit if this is larger than the default value (1.)
    for (const auto& well : wells) {
        const auto& wgroup_name = well.groupName();
            if (wgroup_name == nodeName) {
                if (well.isProducer()) {
                    const auto& pc = well.productionControls(smry);
                    const auto& predMode = well.predictionMode();
                    if (pc.thp_limit >= node_pres) {
                        node_pres = units.from_si(M::pressure, pc.thp_limit);
                        node_wgroup = true;
                    }
                }
            }
        }

    // for nodes that are not well groups, set the node pressure to the fixed pressure potentially higher in the node tree
    if (!node_wgroup) {
    if (fixedPressureNode(sched, nodeName, lookup_step)) {
            // node is a fixed pressure node
            node_pres = units.from_si(M::pressure, network.node(nodeName).terminal_pressure().value());
        }
        else {
            // find fixed pressure higher in the node tree
            bool fp_flag = false;
            auto node_name = nodeName;
            auto upt_br = network.uptree_branch(node_name).value();
            while (!fp_flag) {
                if (fixedPressureNode(sched, upt_br.uptree_node(), lookup_step)) {
                    node_pres = units.from_si(M::pressure, network.node(upt_br.uptree_node()).terminal_pressure().value());
                    fp_flag = true;
                } else {
                    node_name = upt_br.uptree_node();
                    if (network.uptree_branch(node_name).has_value()) {
                        upt_br = network.uptree_branch(node_name).value();
                    } else {
                        std::stringstream str;
                        str << "Node: " << nodeName << " has no uptree node with fixe pressure condition, uppermost node:  " << node_name;
                        throw std::invalid_argument(str.str());
                    }
                }
            }
        }
    }
    return node_pres;
}


namespace INode {
std::size_t entriesPerNode(const std::vector<int>& inteHead)
{
    return inteHead[Opm::RestartIO::Helpers::VectorItems::NINODE];
}

Opm::RestartIO::Helpers::WindowedArray<int>
allocate(const std::vector<int>& inteHead)
{
    using WV = Opm::RestartIO::Helpers::WindowedArray<int>;

    return WV {
        WV::NumWindows{ nodmax(inteHead) },
        WV::WindowSize{ entriesPerNode(inteHead) }
    };
}

int numberOfBranchesConnToNode(const Opm::Schedule& sched, const std::string& nodeName, const size_t lookup_step)
{
    auto& network = sched[lookup_step].network();
    if (network.has_node(nodeName)) {
        int noBranches = network.downtree_branches(nodeName).size();
        noBranches = (network.uptree_branch(nodeName).has_value()) ? noBranches+1 : noBranches;
        return noBranches;
    } else {
        std::stringstream str;
        str << "actual node: " << nodeName << " has not been defined at report time: " << lookup_step+1;
        throw std::invalid_argument(str.str());
    }
}

int cumNumberOfBranchesConnToNode(const Opm::Schedule& sched, const std::string& nodeName, const size_t lookup_step)
{
    auto& network = sched[lookup_step].network();
    std::size_t ind_name = 0;
    int cumNoBranches = 1;
    auto result = findInVector<std::string>(network.insert_index_nd_names(), nodeName);
    if (result.first) {
        ind_name = result.second;
        if (ind_name == 0) {
            return cumNoBranches;
        } else {
            for (std::size_t n_ind = 0; n_ind < ind_name; n_ind++) {
                cumNoBranches += numberOfBranchesConnToNode(sched,  network.insert_index_nd_names()[n_ind], lookup_step);
            }
            return cumNoBranches;
        }
    } else {
        std::stringstream str;
        str << "actual node: " << nodeName << " has not been defined at report time: " << lookup_step+1;
        throw std::invalid_argument(str.str());
    }
}

template <class INodeArray>
void staticContrib(const Opm::Schedule&     sched,
                   const std::string&       nodeName,
                   const std::size_t        lookup_step,
                   INodeArray&              iNode)
{
    //
    iNode[0] = numberOfBranchesConnToNode(sched, nodeName, lookup_step);
    iNode[1] = cumNumberOfBranchesConnToNode(sched, nodeName, lookup_step);
    iNode[2] = sched.getGroup(nodeName, lookup_step).insert_index();
    iNode[3] = (fixedPressureNode(sched, nodeName, lookup_step)) ? 1 : 0;
    iNode[4] = 1;
}

}// Inode

namespace IBran {
std::size_t entriesPerBranch(const std::vector<int>& inteHead)
{
    return inteHead[Opm::RestartIO::Helpers::VectorItems::NIBRAN];
}

Opm::RestartIO::Helpers::WindowedArray<int>
allocate(const std::vector<int>& inteHead)
{
    using WV = Opm::RestartIO::Helpers::WindowedArray<int>;

    return WV {
        WV::NumWindows{ nbrmax(inteHead) },
        WV::WindowSize{ entriesPerBranch(inteHead) }
    };
}

template <class IBranArray>
void staticContrib(const Opm::Schedule&         sched,
                   const Opm::Network::Branch&  branch,
                   const std::size_t            lookup_step,
                   IBranArray&                  iBran)
{
    const auto& nodeNames = sched[lookup_step].network().insert_index_nd_names();

    auto dwntr_nd_res = findInVector<std::string>(nodeNames, branch.downtree_node());
    iBran[0] = (dwntr_nd_res.first) ? dwntr_nd_res.second + 1 : 0 ;

    auto uptr_nd_res = findInVector<std::string>(nodeNames, branch.uptree_node());
    iBran[1] = (uptr_nd_res.first) ? uptr_nd_res.second + 1 : 0 ;

    iBran[2] = (branch.vfp_table().has_value()) ? branch.vfp_table().value() : 0;
}
} // Ibran

namespace INobr {


Opm::RestartIO::Helpers::WindowedArray<int>
allocate(const std::vector<int>& inteHead)
{
    using WV = Opm::RestartIO::Helpers::WindowedArray<int>;

    return WV {
        WV::NumWindows{ 1 },
        WV::WindowSize{ entriesPerInobr(inteHead) }
    };
}

template <class INobrArray>
void staticContrib(const std::vector<int>&   inbr,
                   INobrArray&  iNobr)
{
    for (std::size_t inb = 0; inb < inbr.size(); inb++) {
        iNobr[inb] = inbr[inb];
    }

}
} // Inobr

namespace ZNode {
std::size_t entriesPerZnode(const std::vector<int>& inteHead)
{
    return inteHead[Opm::RestartIO::Helpers::VectorItems::NZNODE];
}

Opm::RestartIO::Helpers::WindowedArray<
Opm::EclIO::PaddedOutputString<8>
>
allocate(const std::vector<int>& inteHead)
{
    using WV = Opm::RestartIO::Helpers::WindowedArray<
               Opm::EclIO::PaddedOutputString<8>
               >;

    return WV {
        WV::NumWindows{ nodmax(inteHead) },
        WV::WindowSize{ entriesPerZnode(inteHead) }
    };
}

template <class ZNodeArray>
void staticContrib(const std::string&       nodeName,
                   ZNodeArray&               zNode)
{
    zNode[0] = nodeName;
}
} // Znode

namespace RNode {
std::size_t entriesPerRnode(const std::vector<int>& inteHead)
{
    return inteHead[Opm::RestartIO::Helpers::VectorItems::NRNODE];
}

Opm::RestartIO::Helpers::WindowedArray<double>
allocate(const std::vector<int>& inteHead)
{
    using WV = Opm::RestartIO::Helpers::WindowedArray<double>;

    return WV {
        WV::NumWindows{ nodmax(inteHead) },
        WV::WindowSize{ entriesPerRnode(inteHead) }
    };
}

template <class RNodeArray>
void dynamicContrib(const Opm::Schedule&      sched,
                    const Opm::SummaryState&  sumState,
                    const std::string&        nodeName,
                    const std::size_t         lookup_step,
                    const Opm::UnitSystem&    units,
                    RNodeArray&               rNode)
{
    const auto& node = sched[lookup_step].network().node(nodeName);

    // node dynamic pressure
    std::string compKey = "GPR:" + nodeName;
    rNode[0] = (sumState.has(compKey)) ? sumState.get(compKey) : 0.;

    // equal to 0. for fixed pressure nodes, 1. otherwise
    rNode[1] = (fixedPressureNode(sched, nodeName, lookup_step)) ? 0. : 1.;

    // equal to i) highest well p_thp if wellgroup and ii) pressure of uptree node with fixed pressure
    rNode[2] = nodePressure(sched, sumState, nodeName, units, lookup_step);

    // fixed value
    rNode[15] = 1.;

}
} // Rnode


namespace RBran {
std::size_t entriesPerRbran(const std::vector<int>& inteHead)
{
    return inteHead[Opm::RestartIO::Helpers::VectorItems::NRBRAN];
}

Opm::RestartIO::Helpers::WindowedArray<double>
allocate(const std::vector<int>& inteHead)
{
    using WV = Opm::RestartIO::Helpers::WindowedArray<double>;

    return WV {
        WV::NumWindows{ nodmax(inteHead) },
        WV::WindowSize{ entriesPerRbran(inteHead) }
    };
}

template <class RBranArray>
void dynamicContrib(const Opm::Schedule&            sched,
                    const Opm::Network::Branch&      branch,
                    const std::size_t                lookup_step,
                    const Opm::SummaryState&         sumState,
                    RBranArray&                      rBran
                   )
{
    // branch (downtree node) rates
    const auto& dwntr_node = branch.downtree_node();
    std::string compKey = "GOPR:" + dwntr_node;
    rBran[0] = (sumState.has(compKey)) ? sumState.get(compKey) : 0.;
    compKey = "GWPR:" + dwntr_node;
    rBran[1] = (sumState.has(compKey)) ? sumState.get(compKey) : 0.;
    compKey = "GGPR:" + dwntr_node;
    rBran[2] = (sumState.has(compKey)) ? sumState.get(compKey) : 0.;
}
} // Rbran





} // namespace



// =====================================================================

Opm::RestartIO::Helpers::AggregateNetworkData::
AggregateNetworkData(const std::vector<int>& inteHead)
    : iNode_ (INode::allocate(inteHead))
    , iBran_ (IBran::allocate(inteHead))
    , iNobr_ (INobr::allocate(inteHead))
    , zNode_ (ZNode::allocate(inteHead))
    , rNode_ (RNode::allocate(inteHead))
    , rBran_ (RBran::allocate(inteHead))
{}

// ---------------------------------------------------------------------


void
Opm::RestartIO::Helpers::AggregateNetworkData::
captureDeclaredNetworkData(const Opm::EclipseState&             es,
                           const Opm::Schedule&                 sched,
                           const Opm::UnitSystem&               units,
                           const std::size_t                    lookup_step,
                           const Opm::SummaryState&             sumState,
                           const std::vector<int>&              inteHead)
{

    auto ntwNdNm = sched[lookup_step].network().insert_index_nd_names();
    std::size_t wdmax = ntwNdNm.size();
    std::vector<const std::string*> ndNmPt(wdmax + 1 , nullptr );
    std::size_t ind = 0;
    for (const auto& nodeName : ntwNdNm) {
        ndNmPt[ind] = &nodeName;
        ind++;
    }

    const auto& networkNodePtrs  =   ndNmPt;
    const auto& branchPtrs = sched[lookup_step].network().branches();

    // Define Static Contributions to INode Array.
    nodeLoop(networkNodePtrs, [&sched, lookup_step, this]
             (const std::string& nodeName, const std::size_t nodeID) -> void
    {
        auto ind = this->iNode_[nodeID];

        INode::staticContrib(sched, nodeName, lookup_step, ind);
    });

    // Define Static Contributions to IBran Array.
    branchLoop(branchPtrs, [&sched, lookup_step, this]
               (const Opm::Network::Branch& branch, const std::size_t branchID) -> void
    {
        auto ib = this->iBran_[branchID];

        IBran::staticContrib(sched, branch, lookup_step, ib);
    });

    // Define Static Contributions to INobr Array
    const std::vector<int> inobr = inobrFunc(sched, lookup_step, inteHead);

    // Define Static Contributions to INobr Array
    if (inobr.size() != entriesPerInobr(inteHead)) {
        std::stringstream str;
        str << "Actual size of inobr:  " << inobr.size() << " different from required size: " << entriesPerInobr(inteHead);
        throw std::invalid_argument(str.str());
    }
    auto i_nobr = this->iNobr_[0];
    INobr::staticContrib(inobr, i_nobr);

    // Define Static Contributions to ZNode Array
    nodeLoop(networkNodePtrs, [this]
             (const std::string& nodeName, const std::size_t nodeID) -> void
    {
        auto ind = this->zNode_[nodeID];

        ZNode::staticContrib(nodeName, ind);
    });

    // Define Static/Dynamic Contributions to RNode Array
    nodeLoop(networkNodePtrs, [&sched, sumState, units, lookup_step, this]
             (const std::string& nodeName, const std::size_t nodeID) -> void
    {
        auto ind = this->rNode_[nodeID];

        RNode::dynamicContrib(sched, sumState, nodeName, lookup_step, units, ind);
    });

    // Define Dynamic Contributions to RBran Array.
    branchLoop(branchPtrs, [&es, &sched, sumState, lookup_step, this]
               (const Opm::Network::Branch& branch, const std::size_t branchID) -> void
    {
        auto ib = this->rBran_[branchID];

        RBran::dynamicContrib(es, sched, branch, lookup_step, sumState, ib);
    });

}
