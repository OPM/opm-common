#!/bin/bash


modDir=$1

echo ${modDir}

if [ -d ${modDir} ] 
then
    echo "Directory /path/to/dir exists." 
#    exit 1    
else
    echo "Directory /path/to/dir does not exists. Making directory"
    mkdir ${modDir}
fi

cp -r ./shared_obj_files ${modDir}/

echo "cp -r ./shared_obj_files ${modDir}/"
mkdir ${modDir}/module

cp -r ./module/*.py ${modDir}/module/


