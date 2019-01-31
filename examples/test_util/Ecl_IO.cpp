#include "Ecl_IO.hpp"

#include <iterator>
#include <iomanip>

bool EclIO::isEOF(std::fstream *fileH){
    
    /* try to read an 4 byte integer, if eof of file passed return true.
       if not eof, seek back to current file possition  */
    
    int num;
    long int pos=fileH->tellg();
    fileH->read((char *)&num, sizeof(num));
    
    if (fileH->eof()){
        return true;
    } else {
        fileH->seekg (pos);
        return false;
    }
}

int EclIO::reverseInt(int num){
    
    int b0,b1,b2,b3;

    b0= (num & 0x000000FF)>>0;
    b1= (num & 0x0000FF00)>>8;
    b2= (num & 0x00FF0000)>>16;
    b3= (num & 0xFF000000)>>24;

    num= (b0<<24) | (b1<<16) | (b2<<8) | (b3<<0) ;

    return num;
}

float EclIO::reverseFloat(float num){
    
    float retVal;
    char *floatToConvert = ( char* ) & num;
    char *returnFloat = ( char* ) & retVal;

    // swap the bytes into a temporary buffer
    returnFloat[0] = floatToConvert[3];
    returnFloat[1] = floatToConvert[2];
    returnFloat[2] = floatToConvert[1];
    returnFloat[3] = floatToConvert[0];

    return retVal;
}

double EclIO::reverseDouble(const double num)
{
    double retVal;
    char *doubleToConvert = ( char* ) & num;
    char *returnDouble = ( char* ) & retVal;

    // swap the bytes into a temporary buffer

    returnDouble[0] = doubleToConvert[7];
    returnDouble[1] = doubleToConvert[6];
    returnDouble[2] = doubleToConvert[5];
    returnDouble[3] = doubleToConvert[4];
    returnDouble[4] = doubleToConvert[3];
    returnDouble[5] = doubleToConvert[2];
    returnDouble[6] = doubleToConvert[1];
    returnDouble[7] = doubleToConvert[0];

    return retVal;
}   

std::vector<std::string> EclIO::split_string(const std::string &instr){

    std::istringstream iss(instr);

    std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>{}};     
 
    return tokens;
}


template <class T> std::string EclIO::make_scientific_string(T number){

    std::stringstream buffer;
    std::string str1,str2,exStr;
    int p1,exp;
    bool isDouble=false;
    
    if (sizeof(number)==8){
        isDouble=true;
    }
    
    if (isDouble==true){
        buffer << std::uppercase << std::scientific << std::setprecision(13) << number << std::nouppercase;
        exStr="D";
    } else {
        buffer << std::uppercase << std::scientific << std::setprecision(7) << number << std::nouppercase;
        exStr="E";
    }
    
    str1=buffer.str();
    p1=str1.find_first_of("E");
    
    str1=str1.substr(0,p1);
    p1=str1.find_first_of(".");
    str1.erase(p1,1); 
    str1.insert(p1-1,"0."); 

    str2=buffer.str();

    p1=str2.find_first_of("E");
    str2=str2.substr(p1+1,str2.length()-p1-1);

    exp=stoi(str2)+1;

    buffer.str("");

    if (abs(exp)<10){
        if (exp < 0){
	    buffer << str1 << exStr << "-0" << abs(exp); 
        } else {
	    buffer << str1 << exStr << "+0" << abs(exp); 
        }
    } else {
        if (exp>0){
	    buffer << str1 << exStr << "+" << exp; 
        } else {
	    buffer << str1 << exStr << exp; 
	}    
    }

    return buffer.str();
}


void EclIO::readBinaryHeader(std::fstream &fileH,std::string &arrName,int &ant,std::string &arrType){

    /* header data 4 byte (value 16), 8 byte (name of array), 4 byte number of elements, 4 byte array type 
     ends with 4 byte (value should be 16 */

    int bhead;
    std::string tmpStrName(8,' ');
    std::string tmpStrType(4,' ');

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    fileH.read((char *)&bhead, sizeof(bhead));
    bhead=reverseInt(bhead);
    
    if (bhead!=16){
        std::string message="Error reading binary header. Expected 16 bytes of header data, found " + std::to_string(bhead);
        throw std::runtime_error(message);
    }
    
    fileH.read(&tmpStrName[0], 8);
    
    fileH.read((char *)&ant, sizeof(ant));
    ant=reverseInt(ant);

    fileH.read(&tmpStrType[0], 4);

    fileH.read((char *)&bhead, sizeof(bhead));
    bhead=reverseInt(bhead);
    
    if (bhead!=16){
        std::string message="Error reading binary header. Expected 16 bytes of header data, found " + std::to_string(bhead);
        throw std::runtime_error(message);
    }
    
    arrName=tmpStrName;
    arrType=tmpStrType;
}


void EclIO::stepOverArray(std::fstream &fileH,const int ant, const std::string arrType){

    long int step=0;
   
    if (arrType=="INTE"){
       
        step=ant*4;
        step=step + ((ant-1) / 1000)*8;    // ignore 8 byte every 1000 value        
       
        if (ant>0){
            step=step+8;
	}
	
    } else if (arrType=="CHAR"){
        step=ant*8;
        step=step + ((ant-1) / 105)*8;  // ignore 8 byte every 105 value 840 bytes         

        if (ant>0){
            step=step+8;
        }
       
    } else if (arrType=="REAL"){
        step=ant*4;
        step=step + ((ant-1) / 1000)*8;  // ignore 8 byte every 1000 value        

        if (ant>0){
            step=step+8;
	}
	
    } else if (arrType=="DOUB"){
        step=ant*8;
        step=step + ((ant-1) / 1000)*8;  // ignore 8 byte every 1000 value        

        if (ant>0){
            step=step+8;
        }
       
    } else if (arrType=="LOGI"){

        step=ant*4;
        step=step + ((ant-1) / 1000)*8;  // ignore 8 byte every 1000 value        

        if (ant>0){
            step=step+8;
	}
    } else if (arrType=="MESS"){

        if (ant>0){
           std::string message="In routine stepOverArray, type MESS should not have size > 0";
           throw std::runtime_error(message);
        }
       
    } else {
        std::string message="Unknown data type, most likely caused by error in reading previous arrays";
        throw std::runtime_error(message);
    }    
    
    if (step>0){      
       fileH.ignore(step);  // step over calculated number of bytes (step)
    } 
}   

std::vector<int> EclIO::readBinaryInteArray(std::fstream &fileH,const int ant){

    std::vector<int> arr;
    int value;
    int num,dhead,dtail,rest;

    rest=ant;

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

   
    while (rest > 0) {   
 
        fileH.read((char *)&dhead, sizeof(dhead));
        dhead=reverseInt(dhead);
   
        num=dhead/4;

        if ((num > 1000) || (num < 0)){
            std::string message="Error reading binary inte data, inconsistent header data or incorrect number of elements";
            throw std::runtime_error(message);
        }
       
        for (int i=0;i<num;i++){
            fileH.read((char *)&value, sizeof(value));
            value=reverseInt(value);
            arr.push_back(value);
        }
       
        rest=rest-num;
       
        if (((num < 1000) && (rest!=0)) || ((num==1000) && (rest < 0))) {
            std::string message="Error reading binary integer data, incorrect number of elements";
            throw std::runtime_error(message);
        }
       
        fileH.read((char *)&dtail, sizeof(dtail));
        dtail=reverseInt(dtail);
       
        if (dhead!=dtail){ 
            std::string message="Error reading binary real data, tail not matching header.";
            throw std::runtime_error(message);
        }
    } 

    return arr;
}

std::vector<float> EclIO::readBinaryRealArray(std::fstream &fileH,const int ant){

    std::vector<float> arr;
    float value;
    int num,dhead,dtail,rest;
   
    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    rest=ant;

    while (rest > 0) {   
 
        fileH.read((char *)&dhead, sizeof(dhead));
        dhead=reverseInt(dhead);
   
        num=dhead/4;
       
        if ((num > 1000) || (num < 0)){
            std::string message="Error reading binary real data, inconsistent header data or incorrect number of elements";
	    throw std::runtime_error(message);
        }
       
        for (int i=0;i<num;i++){
            fileH.read((char *)&value, sizeof(value));
            value=reverseFloat(value);
            arr.push_back(value);
        }
   
        rest=rest-num;
       
        if (((num < 1000) && (rest!=0)) || ((num==1000) && (rest < 0))) {
            std::string message="Error reading binary real data, incorrect number of elements";
            throw std::runtime_error(message);
        }
       
        fileH.read((char *)&dtail, sizeof(dtail));
        dtail=reverseInt(dtail);
 
        if (dhead!=dtail){ 
            std::string message="Error reading binary real data, tail not matching header.";
            throw std::runtime_error(message);
        }
    } 
   
    return arr;
}

std::vector<double> EclIO::readBinaryDoubArray(std::fstream &fileH,const int ant){

    std::vector<double> arr;
    double value;
    int num,dhead,dtail,rest;

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }
   
    rest=ant;

    while (rest > 0) {   
 
        fileH.read((char *)&dhead, sizeof(dhead));
        dhead=reverseInt(dhead);
   
        num=dhead/8;
       
        if ((num > 1000) || (num < 0)){
            std::string message="Error reading binary doub data, inconsistent header data or incorrect number of elements";
	    throw std::runtime_error(message);
        }
       
        for (int i=0;i<num;i++){
            fileH.read((char *)&value, sizeof(value));
            value=reverseDouble(value);
            arr.push_back(value);
        }
   
        rest=rest-num;
       
        if (((num < 1000) && (rest!=0)) || ((num==1000) && (rest < 0))) {
            std::string message="Error reading binary doub data, incorrect number of elements";
            throw std::runtime_error(message);
        }

        fileH.read((char *)&dtail, sizeof(dtail));
        dtail=reverseInt(dtail);
 
        if (dhead!=dtail){ 
            std::string message="Error reading binary doub data, tail not matching header.";
            throw std::runtime_error(message);
        }
    } 
   
    return arr;
}

std::vector<bool> EclIO::readBinaryLogiArray(std::fstream &fileH,const int ant){

    std::vector<bool> arr;
    bool value;
    int num,dhead,dtail,rest;
    unsigned int intVal;

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }
   
    rest=ant;
   
    while (rest > 0) {   
 
        fileH.read((char *)&dhead, sizeof(dhead));
        dhead=reverseInt(dhead);
   
        num=dhead/4;

        if ((num > 1000) || (num < 0)){
            std::string message="Error reading binary logi data, inconsistent header data or incorrect number of elements";
	    throw std::runtime_error(message);
        }

        for (int i=0;i<num;i++){

            fileH.read((char *)&intVal, 4);
           
            if (intVal==0xffffffff){
                value=true;
            } else if (intVal==0x00000000){
                value=false;
            } else {
                std::string message="Error reading log value from element " + std::to_string(i);
                throw std::runtime_error(message);
            }
           
            arr.push_back(value);
        }
   
        rest=rest-num;

        if (((num < 1000) && (rest!=0)) || ((num==1000) && (rest < 0))) {
            std::string message="Error reading binary logi data, incorrect number of elements";
            throw std::runtime_error(message);
        }
       
        fileH.read((char *)&dtail, sizeof(dtail));
        dtail=reverseInt(dtail);
 
        if (dhead!=dtail){ 
            std::string message="Error reading binary logi data, tail not matching header.";
            throw std::runtime_error(message);
        }
    } 
   
    return arr;
}

std::vector<std::string> EclIO::readBinaryCharArray(std::fstream &fileH,const int ant){

    std::string str8(8,' ');
    std::vector<std::string> arr;
   
    int num,dhead,dtail,rest;
   
    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    rest=ant;
   
    while (rest > 0) {   
 
        fileH.read((char *)&dhead, sizeof(dhead));
        dhead=reverseInt(dhead);
        
        num=dhead/8;

        if ((num > 105) || (num < 0)){
            std::string message="Error reading binary char data, inconsistent header data or incorrect number of elements";
	    throw std::runtime_error(message);
        }

        for (int i=0;i<num;i++){
            fileH.read(&str8[0], 8);
            arr.push_back(str8);
        }
   
        rest=rest-num;

        if ((num < 105) && (rest!=0)){
            std::string message="Error reading binary char data, incorrect number of elements";
            throw std::runtime_error(message);
        }
       
        fileH.read((char *)&dtail, sizeof(dtail));
        dtail=reverseInt(dtail);
 
        if (dhead!=dtail){ 
            std::string message="Error reading binary char data, tail not matching header.";
            throw std::runtime_error(message);
        }
    } 
   
    return arr;
}



void EclIO::writeBinaryHeader(std::ofstream &fileH,const std::string arrName,const int ant,const std::string arrType){

    
    /* header data 4 byte (value 16), 8 byte (name of array), 4 byte number of elements, 4 byte array type 
     ends with 4 byte (value should be 16 */

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for writing";
        throw std::runtime_error(message);
    }
    
    int rant=reverseInt(ant);
    int bhead=reverseInt(16);
    
    fileH.write((char *)&bhead, sizeof(bhead));
    
    fileH.write(arrName.c_str(), 8);
    fileH.write((char *)&rant, sizeof(rant));
    fileH.write(arrType.c_str(), 4);

    fileH.write((char *)&bhead, sizeof(bhead));
}


void EclIO::writeBinaryInteArray(std::ofstream &fileH,const std::vector<int> &data){
    
    int rest,num,rval;
    int dhead;

    int n=0;
    int ant=data.size();

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for writing";
        throw std::runtime_error(message);
    }
 
    rest=ant*4;
    
    while (rest>0) {
        
        if (rest>4000){
            rest=rest-4000;
            num=1000;
        } else {
            num=rest/4;
            rest=0;
        }
        
        dhead=reverseInt(num*4);
        
        fileH.write((char *)&dhead, sizeof(dhead));
        
        for (int i=0;i<num;i++){
            rval=reverseInt(data[n]);
            fileH.write((char *)&rval, sizeof(rval));
            n++;    
        }

        fileH.write((char *)&dhead, sizeof(dhead));
    };
}

void EclIO::writeBinaryCharArray(std::ofstream &fileH,const std::vector<std::string> &data){
    
    int num,dhead;

    int n=0;
    int ant=data.size();
    int rest=ant*8;

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for writing";
        throw std::runtime_error(message);
    }
    
    while (rest>0) {
        if (rest>840){
            rest=rest-840;
            num=105;
        } else {
            num=rest/8;
            rest=0;
        }
        
        dhead=reverseInt(num*8);

        fileH.write((char *)&dhead, sizeof(dhead));
        
        for (int i=0;i<num;i++){
            fileH.write(data[n].c_str(), 8);
            n++;    
        }

        fileH.write((char *)&dhead, sizeof(dhead));
    };
}


void EclIO::writeBinaryRealArray(std::ofstream &fileH,const std::vector<float> &data){
    
    int rest,num;
    int dhead;
    float value;
    int n=0;
    int ant=data.size();

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for writing";
        throw std::runtime_error(message);
    }
    
    rest=ant*4;

    while (rest>0) {
        if (rest>4000){
            rest=rest-4000;
            num=1000;
        } else {
            num=rest/4;
            rest=0;
        }
        
        dhead=reverseInt(num*4);

        fileH.write((char *)&dhead, sizeof(dhead));
        
        for (int i=0;i<num;i++){
            value=reverseFloat(data[n]);
            fileH.write((char *)&value, sizeof(value));
            n++;    
        }

        fileH.write((char *)&dhead, sizeof(dhead));
    };
}

void EclIO::writeBinaryDoubArray(std::ofstream &fileH,const std::vector<double> &data){
    
    int rest,num;
    int dhead;
    double value;
    int n=0;
    int ant=data.size();
    
    rest=ant*8;

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for writing";
        throw std::runtime_error(message);
    }
    
    while (rest>0) {
        if (rest>8000){
            rest=rest-8000;
            num=1000;
        } else {
            num=rest/8;
            rest=0;
        }
        
        dhead=reverseInt(num*8);

        fileH.write((char *)&dhead, sizeof(dhead));
        
        for (int i=0;i<num;i++){
            value=reverseDouble(data[n]);
            fileH.write((char *)&value, sizeof(value));
            n++;    
        }

        fileH.write((char *)&dhead, sizeof(dhead));
    };
}


void EclIO::writeBinaryLogiArray(std::ofstream &fileH,const std::vector<bool> &data){
    
    int rest,num;
    int dhead;
    int intVal;
    int n=0;
    int ant=data.size();
    
    rest=ant*4;

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for writing";
        throw std::runtime_error(message);
    }

    while (rest>0) {
        if (rest>4000){
            rest=rest-4000;
            num=1000;
        } else {
            num=rest/4;
            rest=0;
        }
        
        dhead=reverseInt(num*4);

        fileH.write((char *)&dhead, sizeof(dhead));
        
        for (int i=0;i<num;i++){

            if (data[n]==true){
                intVal=0xffffffff;
            } else {
                intVal=0x00000000;
            }

            fileH.write((char *)&intVal, 4);

            n++;    
        }

        fileH.write((char *)&dhead, sizeof(dhead));
    };
}

void EclIO::readFormattedHeader(std::fstream &fileH,std::string &arrName,int &ant,std::string &arrType){

    int p1,p2,p3,p4;
    std::string antStr;
    std::string line;
    
    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    getline (fileH,line);
    
    p1=line.find_first_of("'");
    p2=line.find_first_of("'",p1+1);
    p3=line.find_first_of("'",p2+1);
    p4=line.find_first_of("'",p3+1);
    
    if ((p1==-1) || (p2==-1)|| (p3==-1) || (p4==-1)) {
        std::string message="Header name and type should be enclosed with '";
        throw std::runtime_error(message);
    }
    
    arrName=line.substr(p1+1,p2-p1-1);
    antStr=line.substr(p2+1,p3-p2-1);
    arrType=line.substr(p3+1,p4-p3-1);

    ant=std::stoi(antStr);
 
    if ((arrName.size()!=8) || (arrType.size()!=4))  {
        std::string message="Header name should be 8 characters and array type should be 4 characters";
        throw std::runtime_error(message);
    }
}

std::vector<int> EclIO::readFormattedInteArray(std::fstream &fileH,const int ant){

    std::vector<int> arr;
    std::vector<std::string> tokens;
    std::string line;
    int value;
    int num=0;

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }
   
    while (num<ant) {   
 
        if (fileH.eof()){
            std::string message="End of file reached when reading integer array";
            throw std::runtime_error(message);
         }
       
         getline (fileH,line);
         tokens=split_string(line);

         for (unsigned int i=0;i<tokens.size();i++){
             value=stoi(tokens[i]);
             arr.push_back(value);
         }

         num=num+tokens.size();
    } 

    return arr;
}


std::vector<std::string> EclIO::readFormattedCharArray(std::fstream &fileH,const int ant){

    std::vector<std::string> arr;
    std::string line,value;
    int num=0;
    int p1,p2;

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }
   
    while (num<ant) {   

        getline (fileH,line);
       
        if (line.length()==0){
            std::string message="Reading formatted char array, end of file or blank line, read " + std::to_string(arr.size()) + " of " + std::to_string(ant) + " elements";
            throw std::runtime_error(message);
        }
   
        p1=line.find_first_of("'");
       
        if (p1==-1){
            std::string message="Reading formatted char array, all strings must be enclosed by apostrophe (')";
            throw std::runtime_error(message);
        }
       
        while (p1>-1){ 
            p2=line.find_first_of("'",p1+1);

	    if (p2==-1){
                std::string message="Reading formatted char array, all strings must be enclosed by apostrophe (')";
                throw std::runtime_error(message);
            }
          
	    value=line.substr(p1+1,p2-p1-1);
            arr.push_back(value);
            num++;
          
	    if (value.size()!=8){
                std::string message="Reading formatted char array, all strings should have 8 characters";
                throw std::runtime_error(message);
	    }

            p1=line.find_first_of("'",p2+1);
       }
   } 

   return arr;
}

std::vector<float> EclIO::readFormattedRealArray(std::fstream &fileH,const int ant){

    std::vector<float> arr;
    std::vector<std::string> tokens;
    std::string line;
    int num=0;
    float value;

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }
   
    while (num<ant) {   

        if (fileH.eof()){
            std::string message="End of file reached when reading real array";
            throw std::runtime_error(message);
        }

        getline (fileH,line);
        tokens=split_string(line);
        
        for (unsigned int i=0;i<tokens.size();i++){
	    value=stof(tokens[i]);
            arr.push_back(value);
        }

        num=num+tokens.size();
    } 

    return arr;
}

std::vector<bool> EclIO::readFormattedLogiArray(std::fstream &fileH,const int ant){

    std::vector<bool> arr;
    std::vector<std::string> tokens;
    std::string line;
    int num=0;

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }

    while (num<ant) {   

        if (fileH.eof()){
            std::string message="End of file reached when reading logi array";
            throw std::runtime_error(message);
        }

        getline (fileH,line);
        tokens=split_string(line);
          
        for (unsigned int i=0;i<tokens.size();i++){
          
            if (tokens[i]=="T"){
	        arr.push_back(true);
	    } else if (tokens[i]=="F")   {
	        arr.push_back(false);
	    } else {
                std::string message="Could not convert '" + tokens[i] + "' to a bool value ";
	        throw std::invalid_argument(message);
	    }
        }
       
        num=num+tokens.size();
    } 

    return arr;
}

std::vector<double> EclIO::readFormattedDoubArray(std::fstream &fileH,const int ant){

    std::vector<double> arr;
    std::vector<std::string> tokens;
    std::string line;
    int num,p1;
    double value;
    num=0;

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }
   
    while (num<ant) {   

        if (fileH.eof()){
            std::string message="End of file reached when reading real array";
            throw std::runtime_error(message);
        }

        getline (fileH,line);
        tokens=split_string(line);
       
        for (unsigned int i=0;i<tokens.size();i++){

	    p1=tokens[i].find_first_of("D");

	    if (p1==-1){
                std::string message="Could not convert '" + tokens[i] + "' to double.";
                message=message+" Character D expected instead of E in scientific notation for double. Example 0.28355759043651D+04";
                throw std::invalid_argument(message);
	    }
	  
	    tokens[i].replace(p1,1,"E");
	    value=stod(tokens[i]);
            arr.push_back(value);
        }
        num=num+tokens.size();
    } 

    return arr;
}


void EclIO::writeFormattedHeader(std::ofstream &fileH,const std::string arrName,const int ant,const std::string arrType){

    if (arrName.size()!=8){
        std::string message="Error, input variable arrName should have 8 characters";
        throw std::runtime_error(message);
    }

    if (arrType.size()!=4){
        std::string message="Error, input variable arrType should have 4 characters";
        throw std::runtime_error(message);
    }

    if (fileH.is_open()){
        fileH << " '" << arrName << "' " << std::setw(11) << ant;
        fileH << " '" << arrType << "'" <<  std::endl;
    } else {
        std::string message="ofstream fileH not open for writing";
        throw std::runtime_error(message);
    }
}

void EclIO::writeFormattedInteArray(std::ofstream &fileH,const std::vector<int> &data){

    int ant=data.size();
    int n=0;

    if (!fileH.is_open()){
        std::string message="ofstream fileH not open for writing";
        throw std::runtime_error(message);
    }
    
    for (int i=0;i<ant;i++){
        n++;        
        fileH << std::setw(12) << data[i];   

        if ((n % 6)==0){
           fileH << std::endl;
        }

        if ((n % 1000)==0){
           fileH << std::endl;
           n=0;     
        }
    }

    if (((n % 6)!=0) && ((n % 1000)!=0)) {
        fileH << std::endl;
    }
}


void EclIO::writeFormattedCharArray(std::ofstream &fileH,const std::vector<std::string> &data){

    int ant=data.size();
    std::string str1;
    
    if (!fileH.is_open()){
        std::string message="ofstream fileH not open for writing";
        throw std::runtime_error(message);
    }

    for (int i=0;i<ant;i++){
        str1=data[i];

        if (str1.length()<8){
            str1.insert(str1.length(),8-str1.size(),' ');
        }
       
        fileH << " '" << str1 << "'";

        if (((i+1) % 7)==0){
            fileH << std::endl;
        }
    }

    if ((ant % 7)!=0){
        fileH << std::endl;
    }
}


void EclIO::writeFormattedRealArray(std::ofstream &fileH,const std::vector<float> &data){

    int ant=data.size();

    if (!fileH.is_open()){
        std::string message="ofstream fileH not open for writing";
        throw std::runtime_error(message);
    }
    
    for (int i=0;i<ant;i++){
        if (data[i]==0.0){
            fileH << std::setw(17) << "0.00000000E+00";   
        } else { 
            fileH << std::setw(17) << make_scientific_string(data[i]);   
        }
       
        if (((i+1) % 4)==0){
            fileH << std::endl;
        }
    }

    if ((ant % 4)!=0){
        fileH << std::endl;
    }
}

void EclIO::writeFormattedDoubArray(std::ofstream &fileH,const std::vector<double> &data){

    int ant=data.size();
    int n=0;

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }
    
    for (int i=0;i<ant;i++){
        if (data[i]==0.0){
            fileH << std::setw(23) << "0.00000000000000D+00";   
        } else {    
            fileH << std::setw(23) << make_scientific_string(data[i]);   
        }
       
        n++;
       
        if ((n % 3)==0){
            fileH << std::endl;
        }

        if ((n % 1000)==0){
            fileH << std::endl;
            n=0;     
        }
    }

    if (((n % 3)!=0) && ((n % 1000)!=0)) {
        fileH << std::endl;
    }
}

void EclIO::writeFormattedLogiArray(std::ofstream &fileH,const std::vector<bool> &data){

    int ant=data.size();

    if (!fileH.is_open()){
        std::string message="fstream fileH not open for reading";
        throw std::runtime_error(message);
    }
    
    for (int i=0;i<ant;i++){
        if (data[i]){
            fileH << "  T";
        } else {       
            fileH << "  F";
        }
       
        if ((i+1)%25==0){
            fileH << std::endl;
        }
    }

    if ((ant % 25)!=0){
        fileH << std::endl;
    }
}
