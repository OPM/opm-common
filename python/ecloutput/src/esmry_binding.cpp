#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <src/opm/io/eclipse/EclFile.cpp>
#include <src/opm/io/eclipse/ESmry.cpp>
#include <opm/io/eclipse/EclIOdata.hpp>
#include <src/opm/io/eclipse/EclUtil.cpp>

#include <src/opm/common/OpmLog/OpmLog.cpp>
#include <src/opm/common/OpmLog/LogBackend.cpp>
#include <src/opm/common/OpmLog/LogUtil.cpp>
#include <src/opm/common/OpmLog/StreamLog.cpp>
#include <src/opm/common/OpmLog/Logger.cpp>      

namespace py = pybind11;

class ESmryTmp : public Opm::EclIO::ESmry {
    
public:

    ESmryTmp(const std::string& filename, bool loadBaseRunData): Opm::EclIO::ESmry(filename, loadBaseRunData) {
        time=this->get("TIME");
    };
    
    std::vector<int> getStartDate(){ return startdat; };

    float getLinIt(std::string name, float t){

        std::vector<float> vect = this->get(name);
        
        if ((t < time[0]) || (t > time.back()) || (vect.size()<2)) {
            throw std::invalid_argument("Error, linear interpolation, outside range or length < 2.");
        }

        if (t == time[0]){
            return vect[0];
        } else if (t == time.back()){
            return vect.back();
        } else {
            auto const it = std::lower_bound(time.begin(), time.end(), t);
            int n = std::distance(time.begin(),it);

            return vect[n-1] + (vect[n]-vect[n-1])/(time[n]-time[n-1])*(t-time[n-1]);
        }
    };


    std::vector<float> getLinIt(std::string name, std::vector<float> t_vect){
        
        std::vector<float> resVect;
        
        for (auto v : t_vect){
            resVect.push_back(getLinIt(name, v));
        }
        
        return resVect;
    }

    
    py::array_t<float> get_numpy(const std::string& name) {
        std::vector<float> tmp=get(name);
        return py::array(py::dtype("f"), {tmp.size()}, {}, &tmp[0]);
    };

    py::array_t<float> get_at_rstep_numpy(const std::string& name) {
        std::vector<float> tmp=get_at_rstep(name);
        return py::array(py::dtype("f"), {tmp.size()}, {}, &tmp[0]);
    };
    
    py::array_t<float> getLinItNumpy(std::string name, std::vector<float> t_vect){
        std::vector<float> tmp=getLinIt(name, t_vect);
        return py::array(py::dtype("f"), {tmp.size()}, {}, &tmp[0]);
    }


private:
    
    std::vector<float> time;

    
    
};


PYBIND11_MODULE(esmry_bind, m) {

    py::class_<ESmryTmp>(m, "ESmryBind")
        .def(py::init<const std::string &, bool>())
        .def("hasKey", &ESmryTmp::hasKey)   
        .def("keywordList", &ESmryTmp::keywordList)   
        .def("get", &ESmryTmp::get)   
        
        .def("getLinIt", (float (ESmryTmp::*)(std::string, float)) &ESmryTmp::getLinIt)   
        .def("getLinItList", (std::vector<float> (ESmryTmp::*)(std::string, std::vector<float>)) &ESmryTmp::getLinIt)   
        .def("getLinItNumpy", &ESmryTmp::getLinItNumpy)

        .def("getStartDate", &ESmryTmp::getStartDate)
        
        .def("getNumpy", &ESmryTmp::get_numpy)   
        .def("getAtRstep", &ESmryTmp::get_at_rstep)   
        .def("getAtRstepNumpy", &ESmryTmp::get_at_rstep_numpy)   
        .def("numberOfVectors", &ESmryTmp::numberOfVectors);   
    
}


