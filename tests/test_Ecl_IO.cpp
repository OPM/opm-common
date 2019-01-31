/*
   +   Copyright 2016 Statoil ASA.
   +
   +   This file is part of the Open Porous Media project (OPM).
   +
   +   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   +   the Free Software Foundation, either version 3 of the License, or
   +   (at your option) any later version.
   +
   +   OPM is distributed in the hope that it will be useful,
   +   but WITHOUT ANY WARRANTY; without even the implied warranty of
   +   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   +   GNU General Public License for more details.
   +
   +   You should have received a copy of the GNU General Public License
   +   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   +   */


#include "config.h"
#include <iostream>
#include <stdio.h>
#include <iomanip>    
#include <math.h>  

#include <examples/test_util/Ecl_IO.hpp>


#define BOOST_TEST_MODULE Test Ecl_IO
#include <boost/test/unit_test.hpp>


void writeDummyBinaryFile(std::string fileName){

    // >> xxd -b  <filename> to get hex dump of file

    std::ofstream outFileH;
    std::vector<int> intNumbers={1,2,3,4,5,6,7,8,9,10,11,12};

    outFileH.open(fileName, std::ios::out |  std::ios::binary);
    outFileH << "this is a dummy file, used to test class Ecl_IO";

    for (unsigned int i=0;i<intNumbers.size();i++){
        outFileH << intNumbers[i];
    }

    outFileH.close();
}

void writeDummyFormattedFile(std::string fileName){

    std::ofstream outFileH;

    outFileH.open(fileName, std::ios::out );
    outFileH << "this is a dummy file, used to test class Ecl_IO" << std::endl;;
    outFileH.close();
}


BOOST_AUTO_TEST_CASE(TestBinaryHeader){
  
    std::string tmpFile="TMP1.DAT";
    std::ofstream outFileH;
    std::fstream fileH;
    EclIO file1;

    int ant=10;
    std::string arrName="TESTING ";    // array name needs to be 8 characters length
    std::string arrType="INTE";
  
    // write binary header   

    outFileH.open(tmpFile, std::ios::out |  std::ios::binary);
    file1.writeBinaryHeader(outFileH,arrName,ant,arrType);
    outFileH.close();

    // read binary header   

    int testAnt;
    std::string testArrName;
    std::string testArrType;
  
    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    file1.readBinaryHeader(fileH, testArrName,testAnt,testArrType);
    fileH.close();
  
    // check that elements are same after read back from binary file

    BOOST_CHECK_EQUAL(ant, testAnt);
    BOOST_CHECK_EQUAL(arrName, testArrName);
    BOOST_CHECK_EQUAL(arrType, testArrType);

    // check that exeption is thrown when file is not open for reading
    BOOST_CHECK_THROW(file1.readBinaryHeader(fileH, testArrName,testAnt,testArrType), std::runtime_error );

    // check that exeption is thrown when file is not open for writing
    BOOST_CHECK_THROW(file1.writeBinaryHeader(outFileH,arrName,ant,arrType), std::runtime_error );

    // check that exeption is thrown for inconcistent header data
  
    writeDummyBinaryFile(tmpFile);
  
    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW(file1.readBinaryHeader(fileH, testArrName,testAnt,testArrType), std::runtime_error );
    fileH.close();

    // Delete temporary file

    if (remove(tmpFile.c_str())==-1){
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}

BOOST_AUTO_TEST_CASE(TestFormattedHeader){
  
    std::string tmpFile="TMP1.DAT";
    std::ofstream outFileH;
    std::fstream fileH;
    EclIO file1;

    int testAnt;
    std::string testArrName,testArrType;
  
    int ant=10;
    std::string arrName="TESTING ";    // array name needs to be 8 characters length
    std::string arrType="INTE";        // array type needs to be 4 characters length

    // test write and read back of formatted header   
  
    outFileH.open(tmpFile, std::ios::out );
    file1.writeFormattedHeader(outFileH,arrName,ant,arrType);
    outFileH.close();
  
    fileH.open(tmpFile, std::ios::in );
    file1.readFormattedHeader(fileH, testArrName,testAnt,testArrType);
    fileH.close();
  
    // check that elements are same after read back from formatted file

    BOOST_CHECK_EQUAL(ant, testAnt);
    BOOST_CHECK_EQUAL(arrName, testArrName);
    BOOST_CHECK_EQUAL(arrType, testArrType);

    // ---------------------------------------------------
    // check throws for write formatted header    

    outFileH.open(tmpFile, std::ios::out );

    // check that exeption is thrown when arrayName not equal to 8 characters
    BOOST_CHECK_THROW(file1.writeFormattedHeader(outFileH, "TESTING",10,"INTE"), std::runtime_error );

    // check that exeption is thrown when arrType not equal to 4 characters
    BOOST_CHECK_THROW(file1.writeFormattedHeader(outFileH, "TESTING ",10,"INT"), std::runtime_error );

    outFileH.close();

    // check that exeption is thrown when fileH is not open 
    BOOST_CHECK_THROW(file1.writeFormattedHeader(outFileH, "TESTING ",10,"INTE"), std::runtime_error );

  
    // ---------------------------------------------------
    // check throws for read formatted header    

    // arrType should be enclosed in with '

    outFileH.open(tmpFile, std::ios::out );
    outFileH << " 'TESTING '          10 INTE" << std::endl;
    outFileH.close();

    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW(file1.readFormattedHeader(fileH, testArrName,testAnt,testArrType), std::runtime_error );
    fileH.close();

    // arrName should be enclosed in with '

    outFileH.open(tmpFile, std::ios::out );
    outFileH << " TESTING           10 'INTE'" << std::endl;
    outFileH.close();

    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW(file1.readFormattedHeader(fileH, testArrName,testAnt,testArrType), std::runtime_error );
    fileH.close();

    // invalied integer conversion
  
    outFileH.open(tmpFile, std::ios::out );
    outFileH << " 'TESTING '          xx 'INTE'" << std::endl;
    outFileH.close();

    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW(file1.readFormattedHeader(fileH, testArrName,testAnt,testArrType), std::invalid_argument );
    fileH.close();

    // Header name should be 8 characters
    outFileH.open(tmpFile, std::ios::out );
    outFileH << " 'TESTING'           11 'INTE'" << std::endl;
    outFileH.close();

    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW(file1.readFormattedHeader(fileH, testArrName,testAnt,testArrType), std::runtime_error );
    fileH.close();

    // array type should be 4 characters
    outFileH.open(tmpFile, std::ios::out );
    outFileH << " 'TESTING'           11 'INT'" << std::endl;
    outFileH.close();

    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW(file1.readFormattedHeader(fileH, testArrName,testAnt,testArrType), std::runtime_error );
    fileH.close();
  
    // Delete temporary file

    if (remove(tmpFile.c_str())==-1){
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}



BOOST_AUTO_TEST_CASE(TestBinaryInte){
  
    EclIO file1;
    int nElements=1865;
    std::string tmpFile="TMP1.DAT";
    std::vector<int> intVect;
    std::ofstream outFileH;
    std::fstream fileH;
  
    for (int i=0;i<nElements;i++){
        intVect.push_back(i+1);
    }

    // write integer array to binary file   

    outFileH.open(tmpFile, std::ios::out |  std::ios::binary);
    file1.writeBinaryInteArray(outFileH,intVect);
    outFileH.close();

    // read integer array from binary file   
  
    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    std::vector<int> testIntVect=file1.readBinaryInteArray(fileH,nElements);
    fileH.close();
  
    // check that elements are same after read back from binary file
  
    for (int i=0;i<nElements;i++){
        BOOST_CHECK_EQUAL(intVect[i],  testIntVect[i]);
    }
  
    // check that exeption is thrown if file not open for reading
    BOOST_CHECK_THROW( std::vector<int> testIntVect=file1.readBinaryInteArray(fileH,nElements), std::runtime_error );

    // check that exeption is thrown if file not open for writing
    BOOST_CHECK_THROW( file1.writeBinaryInteArray(outFileH,intVect), std::runtime_error );

    // check that exeption is thrown if wrong number ov elements is given in second argument
    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW( std::vector<int> testIntVect=file1.readBinaryInteArray(fileH,nElements+5), std::runtime_error );
    fileH.close();

    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW( std::vector<int> testIntVect=file1.readBinaryInteArray(fileH,nElements-5), std::runtime_error );
    fileH.close();
  
    // check that exeption is thrown for inconcistent header data
  
    writeDummyBinaryFile(tmpFile);
  
    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW( std::vector<int> testIntVect=file1.readBinaryInteArray(fileH,nElements), std::runtime_error );
    fileH.close();

    // Delete temporary file

    if (remove(tmpFile.c_str())==-1){
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}

BOOST_AUTO_TEST_CASE(TestFormattedInte){
  
    EclIO file1;
    int nElements=1865;
    std::string tmpFile="TMP1.DAT";
    std::vector<int> intVect;
    std::ofstream outFileH;
    std::fstream fileH;
  
    for (int i=0;i<nElements;i++){
        intVect.push_back(i+1);
    }

    // write integer array to binary file   

    outFileH.open(tmpFile, std::ios::out );
    file1.writeFormattedInteArray(outFileH,intVect);
    outFileH.close();

    // read integer array from binary file   
  
    fileH.open(tmpFile, std::ios::in );
    std::vector<int> testIntVect=file1.readFormattedInteArray(fileH,nElements);
    fileH.close();
  
    // check that elements are same after read back from binary file
  
    for (int i=0;i<nElements;i++){
        BOOST_CHECK_EQUAL(intVect[i],  testIntVect[i]);
    }

  
    // check that exeption is thrown if file not open for reading
    BOOST_CHECK_THROW( std::vector<int> testIntVect=file1.readFormattedInteArray(fileH,nElements), std::runtime_error );

    // check that exeption is thrown if file not open for writing
    BOOST_CHECK_THROW( file1.writeFormattedInteArray(outFileH,intVect), std::runtime_error );
  
    // try to read more elements, reaching end of file

    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW( std::vector<int> testIntVect=file1.readFormattedInteArray(fileH,nElements+10), std::runtime_error );
    fileH.close();

    // writing dummary (binary file), string to int conversion thorws invalid exception
    writeDummyBinaryFile(tmpFile);

    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW( std::vector<int> testIntVect=file1.readFormattedInteArray(fileH,nElements), std::invalid_argument );
    fileH.close();
}


BOOST_AUTO_TEST_CASE(TestBinaryReal){
  
    EclIO file1;
    int nElements=1265;
    std::string tmpFile="TMP1.DAT";
    std::vector<float> realVect;
    std::ofstream outFileH;
    std::fstream fileH;
  
    for (int i=0;i<nElements;i++){
        realVect.push_back(float(rand()/(RAND_MAX+1.0)*2500));
    }

    // write real array to binary file   

    outFileH.open(tmpFile, std::ios::out |  std::ios::binary);
    file1.writeBinaryRealArray(outFileH,realVect);
    outFileH.close();

    // read real array from binary file   
  
    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    std::vector<float> testRealVect=file1.readBinaryRealArray(fileH,nElements);
    fileH.close();
  
    // check that elements are same after read back from binary file
  
    for (int i=0;i<nElements;i++){
        BOOST_CHECK_EQUAL(realVect[i],  testRealVect[i]);
    }

    // check that exeption is thrown if file not open for reading
    BOOST_CHECK_THROW( std::vector<float> testRealVect=file1.readBinaryRealArray(fileH,nElements), std::runtime_error );

    // check that exeption is thrown if file not open for writing
    BOOST_CHECK_THROW( file1.writeBinaryRealArray(outFileH,realVect), std::runtime_error );

    // check that exeption is thrown if wrong number ov elements is given in second argument

    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW( std::vector<float> testRealVect=file1.readBinaryRealArray(fileH,nElements+5), std::runtime_error );
    fileH.close();
 
    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW( std::vector<float> testIntVect=file1.readBinaryRealArray(fileH,nElements-5), std::runtime_error );
    fileH.close();
  
    // check that exeption is thrown for inconcistent header data
    writeDummyBinaryFile(tmpFile);

    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW( std::vector<float> testIntVect=file1.readBinaryRealArray(fileH,nElements), std::runtime_error );
    fileH.close();

    // Delete temporary file

    if (remove(tmpFile.c_str())==-1){
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}

BOOST_AUTO_TEST_CASE(TestFormattedReal){

    EclIO file1;
    std::string tmpFile="TMP1.DAT";
    std::ofstream outFileH;
    std::fstream fileH;

    std::stringstream buffer;
    std::vector<float> realVect;
    int nElements=1265;
  
    for (int i=0;i<nElements;i++){
        float number=rand()/(RAND_MAX+1.0);
        int exp = rand() % 70 -35;   
        float val=number*pow(10.0,float(exp));

        buffer.str("");  
        buffer << std::scientific <<  std::setprecision(7) << val;
        float fvalue=stof(buffer.str());
     
        realVect.push_back(fvalue);
    }
  
    // write real array to binary file   

    outFileH.open(tmpFile, std::ios::out );
    file1.writeFormattedRealArray(outFileH,realVect);
    outFileH.close();

    // read real array from binary file   
  
    fileH.open(tmpFile, std::ios::in );
    std::vector<float> testRealVect=file1.readFormattedRealArray(fileH,nElements);
    fileH.close();
  
    // check that elements are same after read back from binary file
  
    for (int i=0;i<nElements;i++){
        BOOST_CHECK_EQUAL(realVect[i],  testRealVect[i]);
    }

    // check that exeption is thrown if file not open for reading
    BOOST_CHECK_THROW( std::vector<float> testRealVect=file1.readFormattedRealArray(fileH,nElements), std::runtime_error );

    // check that exeption is thrown if file not open for writing
    BOOST_CHECK_THROW( file1.writeFormattedRealArray(outFileH,realVect), std::runtime_error );

    // try to read more elements, reaching end of file
    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW( std::vector<float> testRealVect=file1.readFormattedRealArray(fileH,nElements+10), std::runtime_error );
    fileH.close();

    // writing dummary (binary file), string to float conversion thorws invalid exception
    writeDummyBinaryFile(tmpFile);

    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW( std::vector<float> testRealVect=file1.readFormattedRealArray(fileH,nElements), std::invalid_argument );
    fileH.close();
  
    // Delete temporary file

    if (remove(tmpFile.c_str())==-1){
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}


BOOST_AUTO_TEST_CASE(TestBinaryDoub){
  
    EclIO file1;
    int nElements=2002;
    std::string tmpFile="TMP1.DAT";
    std::vector<double> doubVect;
    std::ofstream outFileH;
    std::fstream fileH;
  
    for (int i=0;i<nElements;i++){
        doubVect.push_back(double(rand()/(RAND_MAX+1.0)*2500));
    }

    // write doub array to binary file   

    outFileH.open(tmpFile, std::ios::out |  std::ios::binary);
    file1.writeBinaryDoubArray(outFileH,doubVect);
    outFileH.close();

    // read doub array from binary file   
  
    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    std::vector<double> testDoubVect=file1.readBinaryDoubArray(fileH,nElements);
    fileH.close();
  
    // check that elements are same after read back from binary file
  
    for (int i=0;i<nElements;i++){
        BOOST_CHECK_EQUAL(doubVect[i],  testDoubVect[i]);
    }

    // check that exeption is thrown if file not open for reading
    BOOST_CHECK_THROW( std::vector<double> testDoubVect=file1.readBinaryDoubArray(fileH,nElements), std::runtime_error );

    // check that exeption is thrown if file not open for writing
    BOOST_CHECK_THROW( file1.writeBinaryDoubArray(outFileH,doubVect), std::runtime_error );
  
    // check that exeption is thrown if wrong number ov elements is given in second argument
    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW( std::vector<double> testDoubVect=file1.readBinaryDoubArray(fileH,nElements+5), std::runtime_error );
    fileH.close();

    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW( std::vector<double> testDoubVect=file1.readBinaryDoubArray(fileH,nElements-5), std::runtime_error );
    fileH.close();
  
    // check that exeption is thrown for inconcistent header data

    writeDummyBinaryFile(tmpFile);

    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW( std::vector<double> testDoubVect=file1.readBinaryDoubArray(fileH,nElements), std::runtime_error );
    fileH.close();

    // Delete temporary file

    if (remove(tmpFile.c_str())==-1){
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}

BOOST_AUTO_TEST_CASE(TestFormattedDoub){
  
    EclIO file1;
    int nElements=2002;
    std::string tmpFile="TMP1.DAT";
    std::vector<double> doubVect;
    std::ofstream outFileH;
    std::fstream fileH;
    std::stringstream buffer;
  
  
    for (int i=0;i<nElements;i++){
        double number=rand()/(RAND_MAX+1.0);
        int exp = rand() % 600 - 300;   
        double val=number*pow(10.0,double(exp));
     
        buffer.str("");  
        buffer << std::uppercase << std::scientific << std::setprecision(13) << val << std::nouppercase;
        double dvalue=stod(buffer.str());
        doubVect.push_back(dvalue);
    }

    // write doub array to binary file   

    outFileH.open(tmpFile, std::ios::out |  std::ios::binary);
    file1.writeFormattedDoubArray(outFileH,doubVect);
    outFileH.close();

    // read doub array from binary file   
  
    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    std::vector<double> testDoubVect=file1.readFormattedDoubArray(fileH,nElements);
    fileH.close();
  
    // check that elements are same after read back from binary file
  
    for (int i=0;i<nElements;i++){
        BOOST_CHECK_EQUAL(doubVect[i],  testDoubVect[i]);
    }
  
  
    // check that exeption is thrown if file not open for reading
    BOOST_CHECK_THROW( std::vector<double> testDoubVect=file1.readFormattedDoubArray(fileH,nElements), std::runtime_error );

    // check that exeption is thrown if file not open for writing
    BOOST_CHECK_THROW( file1.writeFormattedDoubArray(outFileH,doubVect), std::runtime_error );

    // try to read more elements, reaching end of file
    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW( std::vector<double> testDoubVect=file1.readFormattedDoubArray(fileH,nElements+10), std::runtime_error );
    fileH.close();

    // writing dummary (formatted file), string to double conversion should thorw std::invalid_argument exception
    writeDummyFormattedFile(tmpFile);

    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW( std::vector<double> testDoubVect=file1.readFormattedDoubArray(fileH,nElements), std::invalid_argument );
    fileH.close();

    // Delete temporary file

    if (remove(tmpFile.c_str())==-1){
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}

BOOST_AUTO_TEST_CASE(TestBinaryLogi){
  
    EclIO file1;
    int nElements=65;
    std::string tmpFile="TMP1.DAT";
    std::vector<bool> logiVect;
    std::ofstream outFileH;
    std::fstream fileH;
  
    for (int i=0;i<nElements;i++){
        double tall=rand()/(RAND_MAX+1.0);

        if (tall>0.5){
            logiVect.push_back(true);
        } else {
            logiVect.push_back(false);
        }
    }
  
    // write logi array to binary file   

    outFileH.open(tmpFile, std::ios::out |  std::ios::binary);
    file1.writeBinaryLogiArray(outFileH,logiVect);
    outFileH.close();

    // read logi array from binary file   
  
    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    std::vector<bool> testLogiVect=file1.readBinaryLogiArray(fileH,nElements);
    fileH.close();
  
    // check that elements are same after read back from binary file
  
    for (int i=0;i<nElements;i++){
        BOOST_CHECK_EQUAL(logiVect[i], testLogiVect[i]);
    }
  
    // check that exeption is thrown if file not open for reading
    BOOST_CHECK_THROW( std::vector<bool> testLogiVect=file1.readBinaryLogiArray(fileH,nElements), std::runtime_error );

    // check that exeption is thrown if file not open for writing
    BOOST_CHECK_THROW( file1.writeBinaryLogiArray(outFileH,logiVect), std::runtime_error );
  
    // check that exeption is thrown if wrong number ov elements is given in second argument

    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW( std::vector<bool> testLogiVect=file1.readBinaryLogiArray(fileH,nElements+5), std::runtime_error );
    fileH.close();

    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW( std::vector<bool> testLogiVect=file1.readBinaryLogiArray(fileH,nElements-5), std::runtime_error );
    fileH.close();

    // check that exeption is thrown for inconcistent header data

    writeDummyBinaryFile(tmpFile);

    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW( std::vector<bool> testLogiVect=file1.readBinaryLogiArray(fileH,nElements), std::runtime_error );
    fileH.close();

    // Delete temporary file

    if (remove(tmpFile.c_str())==-1){
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}

BOOST_AUTO_TEST_CASE(TestFormattedLogi){
  
    EclIO file1;
    int nElements=65;
    std::string tmpFile="TMP1.DAT";
    std::vector<bool> logiVect;
    std::ofstream outFileH;
    std::fstream fileH;
  
    for (int i=0;i<nElements;i++){
        double tall=rand()/(RAND_MAX+1.0);

        if (tall>0.5){
            logiVect.push_back(true);
        } else {
            logiVect.push_back(false);
        }
    }
  
    // write logi array to formatted file   

    outFileH.open(tmpFile, std::ios::out |  std::ios::binary);
    file1.writeFormattedLogiArray(outFileH,logiVect);
    outFileH.close();

    // read logi array from binary file   
  
    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    std::vector<bool> testLogiVect=file1.readFormattedLogiArray(fileH,nElements);
    fileH.close();
  
    // check that elements are same after read back from binary file
  
    for (int i=0;i<nElements;i++){
        BOOST_CHECK_EQUAL(logiVect[i], testLogiVect[i]);
    }

    // check that exeption is thrown if file not open for reading
    BOOST_CHECK_THROW( std::vector<bool> testLogiVect=file1.readFormattedLogiArray(fileH,nElements), std::runtime_error );

    // check that exeption is thrown if file not open for writing
    BOOST_CHECK_THROW( file1.writeFormattedLogiArray(outFileH,logiVect), std::runtime_error );

    // try to read more elements, reaching end of file
    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW( std::vector<bool> testLogiVect=file1.readFormattedLogiArray(fileH,nElements+10), std::runtime_error );
    fileH.close();

    // writing dummary (formatted file), string to logi conversion should thorw std::invalid_argument exception
    writeDummyFormattedFile(tmpFile);

    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW( std::vector<bool> testLogiVect=file1.readFormattedLogiArray(fileH,nElements), std::invalid_argument );
    fileH.close();
  
    // Delete temporary file

    if (remove(tmpFile.c_str())==-1){
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}

BOOST_AUTO_TEST_CASE(TestBinaryChar){
  
    EclIO file1;
    int nElements;
    std::string tmpFile="TMP1.FDAT";
    std::vector<std::string> tmplStrings;
    std::vector<std::string> stringVect;
    std::ofstream outFileH;
    std::fstream fileH;

    tmplStrings={"PROD1   ","A_1_HT2 ","B_1_H   ","F_1_HT4 ","-+-+-+-+","        ","WOPR    "};
  
    for (int i=0;i<42;i++){
       for (unsigned int j=0;j<tmplStrings.size();j++){
         stringVect.push_back(tmplStrings[j]);
       }
    }

    nElements=int(stringVect.size());
  
    // write char array to binary file   

    outFileH.open(tmpFile, std::ios::out |  std::ios::binary);
    file1.writeBinaryCharArray(outFileH,stringVect);
    outFileH.close();

    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    std::vector<std::string> testCharVect=file1.readBinaryCharArray(fileH,nElements);
    fileH.close();
  
    for (int i=0;i<nElements;i++){
        BOOST_CHECK_EQUAL(stringVect[i],  testCharVect[i]);
    }

    // check that exeption is thrown if file not open for reading
    BOOST_CHECK_THROW( std::vector<std::string> testCharVect=file1.readBinaryCharArray(fileH,nElements), std::runtime_error );

    // check that exeption is thrown if file not open for writing
    BOOST_CHECK_THROW( file1.writeBinaryCharArray(outFileH,stringVect), std::runtime_error );

    // check that exeption is thrown if wrong number ov elements is given in second argument

    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW(std::vector<std::string> testCharVect=file1.readBinaryCharArray(fileH,nElements+5), std::runtime_error );
    fileH.close();

    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW(std::vector<std::string> testCharVect=file1.readBinaryCharArray(fileH,nElements-5), std::runtime_error );
    fileH.close();
  
    // check that exeption is thrown for inconcistent header data

    writeDummyBinaryFile(tmpFile);

    fileH.open(tmpFile, std::ios::in |  std::ios::binary);
    BOOST_CHECK_THROW(std::vector<std::string> testCharVect=file1.readBinaryCharArray(fileH,nElements), std::runtime_error );
    fileH.close();
  
    // Delete temporary file

    if (remove(tmpFile.c_str())==-1){
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}


BOOST_AUTO_TEST_CASE(TestFormattedChar){
  
    EclIO file1;
    int nElements;
    std::string tmpFile="TMP1.FDAT";
    std::vector<std::string> tmplStrings;
    std::vector<std::string> stringVect;
    std::ofstream outFileH;
    std::fstream fileH;

    tmplStrings={"PROD1   ","A_1_HT2 ","B_1_H   ","F_1_HT4 ","-+-+-+-+","        ","WOPR    "};
  
    for (int i=0;i<42;i++){
        for (unsigned int j=0;j<tmplStrings.size();j++){
            stringVect.push_back(tmplStrings[j]);
        }
    }

    nElements=int(stringVect.size());
  
    // write char array to binary file   

    outFileH.open(tmpFile, std::ios::out );
    file1.writeFormattedCharArray(outFileH,stringVect);
    outFileH.close();

    fileH.open(tmpFile, std::ios::in );
    std::vector<std::string> testCharVect=file1.readFormattedCharArray(fileH,nElements);
    fileH.close();

  
    for (int i=0;i<nElements;i++){
        BOOST_CHECK_EQUAL(stringVect[i],  testCharVect[i]);
    }

    // check that exeption is thrown if file not open for reading
    BOOST_CHECK_THROW( std::vector<std::string> testCharVect=file1.readFormattedCharArray(fileH,nElements), std::runtime_error );

    // check that exeption is thrown if file not open for writing
    BOOST_CHECK_THROW( file1.writeFormattedCharArray(outFileH,stringVect), std::runtime_error );

    // try to read more elements, reaching end of file
    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW( std::vector<std::string> testCharVect=file1.readFormattedCharArray(fileH,nElements+10), std::runtime_error );
    fileH.close();
  
    // missing apostroph on last variable

    outFileH.open(tmpFile, std::ios::out );
    outFileH << " 'PROD2   ' '        ' '        ' 'INJ1    ' '        ' '         " << std::endl;;
    outFileH.close();
  
    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW( std::vector<std::string> testCharVect=file1.readFormattedCharArray(fileH,6), std::runtime_error );
    fileH.close();

    // missing apostroph on all variable
    outFileH.open(tmpFile, std::ios::out );
    outFileH << "  PROD2                            INJ1                           " << std::endl;;
    outFileH.close();
  
    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW( std::vector<std::string> testCharVect=file1.readFormattedCharArray(fileH,6), std::runtime_error );
    fileH.close();
  
    // string #4 (INJ1) not 8 characters  
    outFileH.open(tmpFile, std::ios::out );
    outFileH << " 'PROD2   ' '        ' '        ' 'INJ1   ' '        ' '         " << std::endl;;
    outFileH.close();
  
    fileH.open(tmpFile, std::ios::in );
    BOOST_CHECK_THROW( std::vector<std::string> testCharVect=file1.readFormattedCharArray(fileH,6), std::runtime_error );
    fileH.close();
  
    // Delete temporary file

    if (remove(tmpFile.c_str())==-1){
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}

BOOST_AUTO_TEST_CASE(TestScientificString){
  
    EclIO file1;

    // testing scientific strings for float type (real arrays)

    BOOST_CHECK_EQUAL("0.98594632E-02",  file1.make_scientific_string(float(9.8594632e-3)));  
    BOOST_CHECK_EQUAL("0.58719699E+09",  file1.make_scientific_string(float(5.8719699e+8)));  
    BOOST_CHECK_EQUAL("0.98145320E+30",  file1.make_scientific_string(float(9.8145320e+29)));  
    BOOST_CHECK_EQUAL("0.60458876E-16",  file1.make_scientific_string(float(6.0458876e-17)));  

    BOOST_CHECK_EQUAL("-0.98594632E-02",  file1.make_scientific_string(float(-9.8594632e-3)));  
    BOOST_CHECK_EQUAL("-0.58719699E+09",  file1.make_scientific_string(float(-5.8719699e+8)));  
    BOOST_CHECK_EQUAL("-0.98145320E+30",  file1.make_scientific_string(float(-9.8145320e+29)));  
    BOOST_CHECK_EQUAL("-0.60458876E-16",  file1.make_scientific_string(float(-6.0458876e-17))); 

    // testing scientific strings for double type (doub arrays)

    BOOST_CHECK_EQUAL("0.72816910455003D-06",  file1.make_scientific_string(double(7.2816910455003e-7))); 
    BOOST_CHECK_EQUAL("0.95177986193448D+08",  file1.make_scientific_string(double(9.5177986193448e+7))); 
    BOOST_CHECK_EQUAL("0.48038829024881D+75",  file1.make_scientific_string(double(4.8038829024881e+74))); 
    BOOST_CHECK_EQUAL("0.10808168631047D-101",  file1.make_scientific_string(double(1.0808168631047e-102))); 

    BOOST_CHECK_EQUAL("-0.72816910455003D-06",  file1.make_scientific_string(double(-7.2816910455003e-7))); 
    BOOST_CHECK_EQUAL("-0.95177986193448D+08",  file1.make_scientific_string(double(-9.5177986193448e+7))); 
    BOOST_CHECK_EQUAL("-0.48038829024881D+75",  file1.make_scientific_string(double(-4.8038829024881e+74))); 
    BOOST_CHECK_EQUAL("-0.10808168631047D-101",  file1.make_scientific_string(double(-1.0808168631047e-102))); 
  
}


BOOST_AUTO_TEST_CASE(TestStepOverArrays){
 
    EclIO file1;

    std::fstream fileH;
    std::string testFile="../../tests/NORNE_ATW2013.INIT";
    std::string arrName,arrType;
    int ant;
  
    fileH.open(testFile, std::ios::in |  std::ios::binary);
  
    while (!file1.isEOF(&fileH)){
        file1.readBinaryHeader(fileH,arrName,ant,arrType);
        file1.stepOverArray(fileH,ant,arrType);
    }  

    BOOST_CHECK_EQUAL(file1.isEOF(&fileH), true);  

    fileH.close();
}
