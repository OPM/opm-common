/*
  Copyright 2025 Equinor ASA.

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

#include <opm/io/eclipse/ERst.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include <cstddef>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <sstream>

class Node
{
public:
    explicit Node(const std::string& name);

    const std::string& name() const
    {
        return m_name;
    }

    void set_outlet(std::shared_ptr<Node> outlet)
    {
        m_outlet = outlet;
    }

    std::shared_ptr<Node> get_outlet()
    {
        return m_outlet;
    }

    void add_inlet_node(std::shared_ptr<Node> n);

    void set_vfp(int vfp)
    {
        m_vfp = vfp;
    }

    int get_vfp()
    {
        return m_vfp;
    }

    void set_fixed_pres(double pres)
    {
        m_fixed_pres = pres;
    }

    double get_fixed_pres()
    {
        return m_fixed_pres;
    }

    int get_xpos()
    {
        return m_xpos;
    }

    void print(std::stringstream& netw_str);

    bool delete_from_inlet_list(const std::string& name);

    bool add_well(const std::string& name);

    void reset_outlet()
    {
        m_outlet = nullptr;
    }

private:
    int m_vfp;
    int m_xpos = -1;
    double m_fixed_pres = -1.0;

    std::string m_name;
    std::shared_ptr<Node> m_outlet;
    std::vector<std::shared_ptr<Node>> m_inlet_list;
    std::vector<std::string> m_well_list;

    std::shared_ptr<Node> next_branch();
};

Node::Node(const std::string& name)
    : m_name(name)
{
    m_vfp = -1;
}

std::shared_ptr<Node>
Node::next_branch()
{
    std::shared_ptr<Node> p1 = m_outlet;

    while (p1 != nullptr) {
        if (std::any_of(p1->m_inlet_list.begin(), p1->m_inlet_list.end(),
                        [](const auto& inlet) { return inlet->m_xpos == -1 ;}))
        {
            return p1;
        }
        p1 = p1->m_outlet;
    }

    return p1;
}

bool
Node::delete_from_inlet_list(const std::string& name)
{
    const auto it = std::find_if(m_inlet_list.begin(), m_inlet_list.end(),
                                 [&name](const auto& inlet) { return inlet->name() == name; });

    if (it == m_inlet_list.end()) {
        return false;
    } else {
        m_inlet_list.erase(it);
        return true;
    }
}

bool
Node::add_well(const std::string& name)
{
    if (!m_inlet_list.empty()) {
        return false;
    }

    m_well_list.push_back(name);

    return true;
}

void
Node::print(std::stringstream& netw_str)
{
    netw_str.seekg(0, std::ios::end);
    int init_length = netw_str.tellg();

    int pos_lineshift = 0;

    auto p = netw_str.str().find_last_of("\n");

    if (p != std::string::npos) {
        pos_lineshift = p + 1;
    }

    if (m_outlet == nullptr) {
        netw_str << "  o (" << m_name << ")";
        m_xpos = netw_str.str().find_first_of("o", init_length) - pos_lineshift;
    } else if (m_vfp == 9999) {
        netw_str << " --- +(" << m_name << ")";
        m_xpos = netw_str.str().find_first_of("+", init_length) - pos_lineshift;
    } else {
        netw_str << " --[" << m_vfp << "]-- +(" << m_name << ")";
        m_xpos = netw_str.str().find_first_of("+", init_length) - pos_lineshift;
    }

    for (std::size_t m = 0; m < m_inlet_list.size(); m++) {
        m_inlet_list[m]->print(netw_str);
    }

    if (m_inlet_list.size() == 0) {
        auto next_br = next_branch();

        netw_str << " : ";

        for (const auto& well : m_well_list) {
            netw_str << " " << well;
        }

        if (next_br != nullptr) {
            std::string xpos_str(next_br->m_xpos, ' ');
            netw_str << "\n" << xpos_str << "\\\n" << xpos_str;
        }
    }
}

void
Node::add_inlet_node(std::shared_ptr<Node> node)
{
    std::vector<std::shared_ptr<Node>>::iterator exist;

    exist = std::find_if(m_inlet_list.begin(), m_inlet_list.end(),
                         [&](const auto& val) { return val->name() == node->name(); });

    if (exist != m_inlet_list.end()) {
        m_inlet_list.erase(exist);
    }

    m_inlet_list.push_back(node);
}

class NetWork
{
public:
    // upptree, downtree, vfp
    using bran_input_type = std::tuple<std::string, std::string, int>;

    // name and fixed pressure
    using node_input_type = std::tuple<std::string, int>;

    // well and group name
    using well_input_type = std::tuple<std::string, std::string>;

    explicit NetWork(const std::string& filename);

    void print_report_steps();
    int number_report_steps()
    {
        return m_report_time_list.size();
    }

    void build_network(int rstep);
    void print_network(int rstep);

private:
    time_t time_from_rec(const Opm::DeckRecord& rec);
    time_t time_from_rst(const std::string& rstfile, int rstep);

    bool node_exist(const std::string& name);

    void add_node(const std::string& name);
    void add_branch(const std::string& downtree, const std::string& uptree, int vfp);
    void delete_branch(const std::string& downtree, const std::string& uptree);

    void br_input_from_rst(const std::string& rstfile,
                           const std::vector<int>& rstep_vect);

    void parse_data_deck(const std::filesystem::path& inputFileName);
    void parse_unrst(const std::filesystem::path& inputFileName);

    std::stringstream m_netw_str;

    time_t m_start_date;
    time_t m_rst_time;

    bool m_from_unrst = false;

    std::vector<time_t> m_report_time_list;
    std::vector<std::vector<bran_input_type>> m_bran_input_list;
    std::vector<std::vector<node_input_type>> m_node_input_list;
    std::vector<std::vector<well_input_type>> m_well_input_list;

    std::string time_str(time_t t1);

    std::vector<std::shared_ptr<Node>> m_node_list;
    std::vector<std::shared_ptr<Node>> m_top_node_list;
};

NetWork::NetWork(const std::string& filename)
{
    std::filesystem::path inputFileName {filename};

    if (inputFileName.extension() == ".DATA") {
        parse_data_deck(inputFileName);
    }
    else if (inputFileName.extension() == ".UNRST") {
        parse_unrst(inputFileName);
        m_from_unrst = true;
    }
    else {
        std::cout << "\n!Error, unsupported file type " << filename << "\n\n";
        exit(1);
    }
}

void
NetWork::parse_data_deck(const std::filesystem::path& inputFileName)
{
    Opm::ParseContext parseContext;
    parseContext.update(Opm::ParseContext::PARSE_UNKNOWN_KEYWORD, Opm::InputErrorAction::IGNORE);
    parseContext.update(Opm::ParseContext::PARSE_RANDOM_TEXT, Opm::InputErrorAction::IGNORE);
    parseContext.update(Opm::ParseContext::PARSE_EXTRA_RECORDS, Opm::InputErrorAction::IGNORE);
    parseContext.update(Opm::ParseContext::PARSE_RANDOM_SLASH, Opm::InputErrorAction::IGNORE);

    std::vector<Opm::Ecl::SectionType> sections = {Opm::Ecl::RUNSPEC, Opm::Ecl::SOLUTION, Opm::Ecl::SCHEDULE};

    Opm::Parser parser;

    Opm::Deck deck_schecule;

    try {
        deck_schecule = parser.parseFile(inputFileName, parseContext, sections);
    }
    catch (const std::exception& e) {
        std::cout << "\n!Error parsing data deck " << inputFileName << "\n\n";
        std::cout << e.what() << "\n\n";

        exit(1);
    }

    bool restart = false;
    bool skiprest = false;

    time_t last_time;

    auto network_keyw = deck_schecule["NETWORK"];

    if (network_keyw.size() == 0) {
        std::cout << "\n > !Error, data deck " << inputFileName << " doesn't include a production network \n\n";
        exit(1);
    }

    for (const auto& keyw : deck_schecule) {
        if (keyw.name() == "START") {
            m_node_input_list.push_back({});
            m_bran_input_list.push_back({});
            m_well_input_list.push_back({});
            m_start_date = time_from_rec(keyw[0]);

            last_time = m_start_date;
        }
        else if (keyw.name() == "TSTEP") {
            for (std::size_t n = 0; n < keyw[0].getItem(0).data_size(); n++) {
                auto dt = keyw[0].getItem(0).get<double>(n);
                last_time = last_time + static_cast<int>(dt * 24.0 * 3600.0);

                if (!skiprest) {
                    m_report_time_list.push_back(last_time);
                    m_node_input_list.push_back({});
                    m_bran_input_list.push_back({});
                    m_well_input_list.push_back({});
                }

                if ((skiprest) && (last_time >= m_rst_time)) {
                    skiprest = false;
                }
            }
        }
        else if (keyw.name() == "DATES") {
            for (const auto& rec : keyw) {
                last_time = time_from_rec(rec);

                if (m_report_time_list.size() == 0) {
                    if ((last_time <= m_start_date) && (!skiprest)) {
                        std::cout << "\n!Error, next report step '" << time_str(last_time)
                                  << "' has already passed \n\n";
                        exit(1);
                    }
                }
                else {
                    if ((last_time <= m_report_time_list.back()) && (!skiprest)) {
                        std::cout << "\n!Error, next report step '" << time_str(last_time)
                                  << "' has already passed \n\n";
                        exit(1);
                    }
                }

                if (!skiprest) {
                    m_report_time_list.push_back(last_time);
                    m_node_input_list.push_back({});
                    m_bran_input_list.push_back({});
                    m_well_input_list.push_back({});
                }

                if ((skiprest) && (last_time >= m_rst_time)) {
                    skiprest = false;
                }
            }
        }
        else if (keyw.name() == "RESTART") {
            auto rst_file = keyw[0].getItem(0).get<std::string>(0) + ".UNRST";
            auto rst_rstep = keyw[0].getItem(1).get<int>(0);

            br_input_from_rst(rst_file, {rst_rstep});

            restart = true;
        }
        else if (keyw.name() == "SKIPREST") {
            if (restart) {
                skiprest = true;
            }
        }
        else if ((keyw.name() == "BRANPROP") && (!skiprest)) {
            for (const auto& rec : keyw) {
                auto downtree = rec.getItem(0).get<std::string>(0);
                auto uptree = rec.getItem(1).get<std::string>(0);
                auto vfp = rec.getItem(2).get<int>(0);

                auto br = std::make_tuple(downtree, uptree, vfp);

                m_bran_input_list.back().push_back(br);
            }
        }
        else if ((keyw.name() == "NODEPROP") && (!skiprest)) {
            for (const auto& rec : keyw) {
                if (rec.getItem(1).hasValue(0)) {
                    auto node_name = rec.getItem(0).get<std::string>(0);
                    auto node_pres = rec.getItem(1).get<double>(0);
                    auto node = std::make_tuple(node_name, node_pres);
                    m_node_input_list.back().push_back(node);
                }
            }
        }
        else if ((keyw.name() == "WELSPECS") && (!skiprest)) {
            for (const auto& rec : keyw) {
                auto wname = rec.getItem(0).get<std::string>(0);
                auto gname = rec.getItem(1).get<std::string>(0);

                auto well = std::make_tuple(wname, gname);

                m_well_input_list.back().push_back(well);
            }
        }
    }
}

void
NetWork::parse_unrst(const std::filesystem::path& inputFileName)
{
    Opm::EclIO::ERst rst1(inputFileName);

    auto all_reports = rst1.listOfReportStepNumbers();

    std::vector<int> rstep_vect;

    std::copy_if(all_reports.begin(), all_reports.end(),
                 std::back_inserter(rstep_vect),
                 [](const auto r) { return r > 0; });

    m_node_input_list.push_back({});
    m_bran_input_list.push_back({});
    m_well_input_list.push_back({});

    br_input_from_rst(inputFileName, rstep_vect);
}

std::string
NetWork::time_str(time_t t1)
{
    const std::vector<std::string> mndStr {
        "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
    };

    const std::tm* date = std::localtime(&t1);

    std::stringstream date_str;

    // ignoring daylight saving time

    int hr = date->tm_hour;
    int day = date->tm_mday;

    if (date->tm_isdst == 1) {
        hr = hr - 1;
        if (hr < 0) {
            hr = hr + 24;
            day = day - 1;
        }
    }

    date_str << std::setw(2) << std::setfill('0') << day;
    date_str << " '" << mndStr[date->tm_mon] << "' " << date->tm_year + 1900;
    date_str << " " << std::setw(2) << std::setfill('0') << hr;
    date_str << ":" << std::setw(2) << std::setfill('0') << date->tm_min;
    date_str << ":" << std::setw(2) << std::setfill('0') << date->tm_sec;

    return date_str.str();
}

bool
NetWork::node_exist(const std::string& name)
{
    return std::any_of(m_node_list.begin(), m_node_list.end(),
                       [&name](const auto& node) { return node->name() == name; });
}

void
NetWork::add_node(const std::string& name)
{
    if (!this->node_exist(name)) {
        m_node_list.push_back(std::make_shared<Node>(name));
    }
    else {
        std::cout << "in function add_node: Node " << name << " already exists \n\n";
        exit(1);
    }
}

void
NetWork::add_branch(const std::string& downtree, const std::string& uptree, int vfp)
{
    if (vfp == 0) {
        std::cout << "\n!Error, vfp = 0, use function remove_branch to remove a branch \n\n";
        exit(1);
    }

    std::shared_ptr<Node> pUptree;
    std::shared_ptr<Node> pDowntree;

    // handle uptree node
    for (std::size_t n = 0; n < m_node_list.size(); n++) {
        if (m_node_list[n]->name() == uptree) {
            pUptree = m_node_list[n];
        }
    }

    if (pUptree == nullptr) {
        add_node(uptree);
        pUptree = m_node_list.back();
    }

    // handle down tree node
    for (std::size_t n = 0; n < m_node_list.size(); n++) {
        if (m_node_list[n]->name() == downtree) {
            pDowntree = m_node_list[n];
        }
    }

    if (pDowntree == nullptr) {
        add_node(downtree);
        pDowntree = m_node_list.back();
    }

    pDowntree->set_outlet(pUptree);
    pDowntree->set_vfp(vfp);

    pUptree->add_inlet_node(pDowntree);
}

void
NetWork::delete_branch(const std::string& downtree, const std::string& uptree)
{
    auto up_it = std::find_if(m_node_list.begin(), m_node_list.end(),
                           [&uptree](const auto& node) { return node->name() == uptree;});

    auto down_it = std::find_if(m_node_list.begin(), m_node_list.end(),
                                [&downtree](const auto& node) { return node->name() == downtree;});

    if (up_it == m_node_list.end() || down_it == m_node_list.end()) {
        std::cout << "\n!Error, pointer to downtree and/or uptree not found \n\n";
        exit(1);
    }

    if ((*up_it)->delete_from_inlet_list(downtree) == false) {
        std::cout << "\n!Error, problem with deleteing branch, needs to be checked  \n\n";
        exit(1);
    }

    (*down_it)->reset_outlet();

    const auto top_it = std::find_if(m_top_node_list.begin(), m_top_node_list.end(),
                                     [&downtree](const auto& node) { return node->name() == downtree; });

    if (top_it == m_top_node_list.end()) {
        m_top_node_list.push_back(*up_it);
    }
}

void
NetWork::build_network(int rstep)
{
    std::vector<int> input_step_vect;

    // input from restart file -> each report step describes a complete state of network
    // input from data deck -> network build from all steps up to report step (branprop increments)

    if (m_from_unrst) {
        input_step_vect.push_back(rstep - 1);
    }
    else {
        for (int n = 0; n < rstep; n++) {
            input_step_vect.push_back(n);
        }
    }

    for (const auto n : input_step_vect) {
        for (std::size_t b = 0; b < m_bran_input_list[n].size(); b++) {
            auto downtree = std::get<0>(m_bran_input_list[n][b]);
            auto uptree = std::get<1>(m_bran_input_list[n][b]);
            auto vfp = std::get<2>(m_bran_input_list[n][b]);

            if (vfp == 0) {
                delete_branch(downtree, uptree);
            }
            else {
                add_branch(downtree, uptree, vfp);
            }
        }
    }

    m_top_node_list.clear();

    for (std::size_t n = 0; n < m_node_list.size(); n++) {
        if (m_node_list[n]->get_outlet() == nullptr) {
            m_top_node_list.push_back(m_node_list[n]);
        }
    }

    std::map<std::string, std::string> well_map;

    for (const auto n : input_step_vect) {
        for (std::size_t b = 0; b < m_well_input_list[n].size(); b++) {
            auto wname = std::get<0>(m_well_input_list[n][b]);
            auto gname = std::get<1>(m_well_input_list[n][b]);
            well_map[wname] = gname;
        }
    }

    for (const auto& m : well_map) {
        auto gname = m.second;
        auto wname = m.first;

        for (std::size_t n = 0; n < m_node_list.size(); n++) {
            if (m_node_list[n]->name() == gname) {
                m_node_list[n]->add_well(wname);
            }
        }
    }

    for (const auto n : input_step_vect) {
        for (std::size_t b = 0; b < m_node_input_list[n].size(); b++) {
            auto node = std::get<0>(m_node_input_list[n][b]);
            auto pressure = std::get<1>(m_node_input_list[n][b]);

            for (std::size_t m = 0; m < m_node_list.size(); m++) {
                if (m_node_list[m]->name() == node) {
                    m_node_list[m]->set_fixed_pres(pressure);
                }
            }
        }
    }
}

void
NetWork::print_network(int rstep)
{
    std::cout << "\n\n";

    time_t t = m_report_time_list[rstep - 1];

    std::cout << "Report step : " << time_str(t) << "\n\n";

    for (std::size_t n = 0; n < m_top_node_list.size(); n++) {
        m_top_node_list[n]->print(m_netw_str);
        m_netw_str << "\n\n";
    }

    std::cout << "\n" << m_netw_str.str() << "";

    std::cout << "\nFixed pressure nodes: \n\n";

    for (std::size_t n = 0; n < m_node_list.size(); n++) {
        auto fixed_pres = m_node_list[n]->get_fixed_pres();

        if (fixed_pres > -1.0) {
            std::cout << "  " << m_node_list[n]->name() << " = ";
            std::cout << std::fixed << std::setprecision(2) << fixed_pres;
        }
    }

    std::cout << "\n\n\n";
}

void
NetWork::print_report_steps()
{
    if (!m_from_unrst) {
        std::cout << "\n\nStart date  " << time_str(m_start_date) << "\n";
    }

    std::cout << "\nList of all report steps \n\n";

    for (std::size_t n = 0; n < m_report_time_list.size(); n++) {
        std::cout << "Report step " << n + 1 << "  | " << time_str(m_report_time_list[n]) << "\n";
    }
}

time_t
NetWork::time_from_rec(const Opm::DeckRecord& rec)
{
    const std::map<std::string, int> mnd_map {
        {"JAN", 0},
        {"FEB", 1},
        {"MAR", 2},
        {"APR", 3},
        {"MAY", 4},
        {"JUN", 5},
        {"JUL", 6},
        {"JLY", 6},
        {"AUG", 7},
        {"SEP", 8},
        {"OCT", 9},
        {"NOV", 10},
        {"DEC", 11}
    };

    auto day = rec.getItem(0).get<int>(0);
    auto mndStr = rec.getItem(1).get<std::string>(0);
    auto year = rec.getItem(2).get<int>(0);
    auto time = rec.getItem(3).get<std::string>(0);

    int sec_frac = 0;

    auto p1 = time.find(".");

    if (p1 != std::string::npos) {
        auto sec_frac_str = time.substr(p1 + 1);
        sec_frac = std::stoi(sec_frac_str);
        time = time.substr(0, p1);
    }

    if (sec_frac > 0) {
        std::cout << "\n!Error, fraction of section not supported \n";
        exit(1);
    }

    p1 = time.find(":");

    if (p1 == std::string::npos) {
        std::cout << "\n!Error, invalied format for time " << time << "\n";
        exit(1);
    }

    auto p2 = time.find(":", p1 + 1);
    if (p2 == std::string::npos) {
        std::cout << "\n!Error, Second invalied format for time " << time << "\n";
        exit(1);
    }

    int h = std::stoi(time.substr(0, p1));
    int min = std::stoi(time.substr(p1 + 1, p2 - p1 - 1));
    int sec = std::stoi(time.substr(p2 + 1));

    int mnd = mnd_map.at(mndStr);

    // std::tm date1 = {sec, min, h, day, mnd, year - 1900 };

    std::tm date1 = {};

    date1.tm_sec = sec;
    date1.tm_min = min;
    date1.tm_hour = h;

    date1.tm_mday = day;
    date1.tm_mon = mnd;
    date1.tm_year = year - 1900;

    return std::mktime(&date1);
}

time_t
NetWork::time_from_rst(const std::string& rstfile, int rstep)
{
    Opm::EclIO::ERst rst1(rstfile);

    auto inteh = rst1.getRestartData<int>("INTEHEAD", rstep);

    int year = inteh[66];
    int mnd = inteh[65];
    int day = inteh[64];

    int h = inteh[206];
    int min = inteh[207];
    int sec = static_cast<int>(inteh[410] / 1000000);

    std::tm date1 = {};

    date1.tm_sec = sec;
    date1.tm_min = min;
    date1.tm_hour = h;

    date1.tm_mday = day;
    date1.tm_mon = mnd - 1;
    date1.tm_year = year - 1900;

    return std::mktime(&date1);
}

void
NetWork::br_input_from_rst(const std::string& rstfile,
                           const std::vector<int>& rstep_vect)
{
    Opm::EclIO::ERst rst1(rstfile);

    for (const int rstep : rstep_vect) {
        m_rst_time = time_from_rst(rstfile, rstep);
        m_report_time_list.push_back(m_rst_time);

        auto intehead = rst1.getRestartData<int>("INTEHEAD", rstep);

        if (rst1.hasArray("ZNODE", rstep)) {
            std::vector<std::string> nodelist;

            auto noactnod = intehead[129]; // Number of active/defined nodes in the network
            auto nibran = intehead[133]; // number of entries per branch in the IBRAN array
            auto noactbr = intehead[130]; // Number of active/defined branches in the network
            auto nrnode = intehead[136]; // number of entries per node in the RNODE array

            auto znode = rst1.getRestartData<std::string>("ZNODE", rstep);
            auto ibran = rst1.getRestartData<int>("IBRAN", rstep);
            auto rnode = rst1.getRestartData<double>("RNODE", rstep);

            for (int n = 0; n < noactnod; n++) {
                nodelist.push_back(znode[2 * n]);
            }

            for (int b = 0; b < noactbr; b++) {
                int ind = b * nibran;
                std::string downtree = nodelist[ibran[ind] - 1];
                std::string uptree = nodelist[ibran[ind + 1] - 1];
                int vfp = ibran[ind + 2];

                auto br = std::make_tuple(downtree, uptree, vfp);
                m_bran_input_list.back().push_back(br);
            }

            for (int n = 0; n < noactnod; n++) {
                int ind = n * nrnode;
                if (rnode[ind + 1] == 0.0) {
                    auto node = std::make_tuple(nodelist[n], rnode[ind + 2]);
                    m_node_input_list.back().push_back(node);
                }
            }
        }

        std::vector<std::string> grouplist;

        auto nzwelz = intehead[27]; // Number of 8-character words per well in ZWEL array
        auto nswells = intehead[16]; // Number of wells
        auto nzgrpz = intehead[39]; // Number of data elements per group in ZGRP array
        auto ngmaxz = intehead[20]; // Maximum number of groups in field
        auto niwelz = intehead[24]; // Number of data elements per well in IWEL array

        auto zwel = rst1.getRestartData<std::string>("ZWEL", rstep);
        auto iwel = rst1.getRestartData<int>("IWEL", rstep);
        auto zgrp = rst1.getRestartData<std::string>("ZGRP", rstep);

        for (int g = 0; g < ngmaxz; g++) {
            grouplist.push_back(zgrp[g * nzgrpz]);
        }

        for (int n = 0; n < nswells; n++) {
            std::string wname = zwel[n * nzwelz];
            int grp_ind = iwel[n * niwelz + 5] - 1;
            std::string gname = grouplist[grp_ind];
            auto well = std::make_tuple(wname, gname);
            m_well_input_list.back().push_back(well);
        }

        m_node_input_list.push_back({});
        m_bran_input_list.push_back({});
        m_well_input_list.push_back({});
    }
}

static void
printHelp()
{
    std::cout << "\n This program visualizes a production network with terminal output."
              << " Input to this program should be a valid data deck (.DATA) \n or a unified"
              << " restart file (.UNRST).\n\n The program takes these options"
              << " (which must be given before the arguments):\n\n"
              << " -l lists all available report steps and exit.\n"
              << " -r selects report step to be visualized. Default is the last report step \n"
              << " -h Print help and exit.\n\n";
}

int
main(int argc, char** argv)
{
    int c = 0;
    bool list_report_steps = false;
    int rstep = -1;

    while ((c = getopt(argc, argv, "lr:h")) != -1) {
        switch (c) {
        case 'l':
            list_report_steps = true;
            break;
        case 'h':
            printHelp();
            return 0;
        case 'r':
            rstep = atoi(optarg);
            break;
        default:
            return EXIT_FAILURE;
        }
    }

    int argOffset = optind;

    NetWork netw(argv[argOffset]);

    if (list_report_steps) {
        netw.print_report_steps();
        std::cout << "\n";

        return 0;
    }

    if (rstep == -1) {
        rstep = netw.number_report_steps();
    }
    else {
        if ((rstep < 1) || (rstep > netw.number_report_steps())) {
            std::cout << "\n!Error, invalid report step " << rstep;
            std::cout << " should be > 0 and less than " << netw.number_report_steps() << "\n";
            std::cout << "        use option -l to list all report steps. " << "\n\n";
            exit(1);
        }
    }

    netw.build_network(rstep);
    netw.print_network(rstep);

    std::cout << "\n\n";

    return EXIT_SUCCESS;
}
