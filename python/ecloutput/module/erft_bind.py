import sys
import os
import imp

libDir=os.path.dirname(__file__)

pyversion=sys.version.split()[0]
tokens=pyversion.split(".")

major=tokens[0]
minor=tokens[1]


if major=="2" and minor=="7":
    libDir=os.path.join(libDir,"../shared_obj_files/python2.7")
    so_file=os.path.join(libDir,"erft.so")
elif major=="3" and minor=="6":
    libDir=os.path.join(libDir,"../shared_obj_files/python3.6")
    so_file=os.path.join(libDir,"erft.cpython-36m-x86_64-linux-gnu.so")

else:
    print ("Python version %s not supported by module eclOutput")
    exit(1)


def load_module(name):
    global __loader__
    __loader__ = None
    del  __loader__
    imp.load_dynamic(name, so_file)

      
   
load_module('erft_bind')
