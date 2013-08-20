/* 
 * File:   EclipseDataDoctor.cpp
 * Author: kflik
 *
 * Created on August 20, 2013, 1:19 PM
 */

#include <iostream>

/*
 * 
 */
int main(int argc, char** argv) {
    if (argc <= 1)
    {
        std::cout << "Usage: " << argv[0] << " <Filename>" << std::endl;
        exit(1);
    }
    return 0;
}

