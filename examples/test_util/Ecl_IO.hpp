#include <iostream>
#include <fstream>
#include <vector>


class EclIO // 
{

public:
    int reverseInt(int num);
    float reverseFloat(float num);
    double reverseDouble(const double num);

    template <class T> std::string make_scientific_string(T number);
    std::vector<std::string> split_string(const std::string &instr);

    bool isEOF(std::fstream *fileH);

    void stepOverArray(std::fstream &fileH,const int ant, const std::string arrType);
    
    void readBinaryHeader(std::fstream &fileH,std::string &arrName,int &ant,std::string &arrType);
    std::vector<int> readBinaryInteArray(std::fstream &fileH,const int ant);
    std::vector<float> readBinaryRealArray(std::fstream &fileH,const int ant);
    std::vector<double> readBinaryDoubArray(std::fstream &fileH,const int ant);
    std::vector<bool> readBinaryLogiArray(std::fstream &fileH,const int ant);
    std::vector<std::string> readBinaryCharArray(std::fstream &fileH,const int ant);

    void writeBinaryHeader(std::ofstream &fileH,const std::string arrName,const int ant,const std::string arrType);
    void writeBinaryInteArray(std::ofstream &fileH,const std::vector<int> &data);
    void writeBinaryCharArray(std::ofstream &fileH,const std::vector<std::string> &data);
    void writeBinaryRealArray(std::ofstream &fileH,const std::vector<float> &data);
    void writeBinaryDoubArray(std::ofstream &fileH,const std::vector<double> &data);
    void writeBinaryLogiArray(std::ofstream &fileH,const std::vector<bool> &data);
    
    void readFormattedHeader(std::fstream &fileH,std::string &arrName,int &ant,std::string &arrType);
    std::vector<int> readFormattedInteArray(std::fstream &fileH,const int ant);
    std::vector<std::string> readFormattedCharArray(std::fstream &fileH,const int ant);
    std::vector<float> readFormattedRealArray(std::fstream &fileH,const int ant);
    std::vector<bool> readFormattedLogiArray(std::fstream &fileH,const int ant);
    std::vector<double> readFormattedDoubArray(std::fstream &fileH,const int ant);
    
    void writeFormattedHeader(std::ofstream &fileH,const std::string arrName,const int ant,const std::string arrType);
    void writeFormattedInteArray(std::ofstream &fileH,const std::vector<int> &data);
    void writeFormattedCharArray(std::ofstream &fileH,const std::vector<std::string> &data);
    void writeFormattedRealArray(std::ofstream &fileH,const std::vector<float> &data);
    void writeFormattedLogiArray(std::ofstream &fileH,const std::vector<bool> &data);
    void writeFormattedDoubArray(std::ofstream &fileH,const std::vector<double> &data);
};
