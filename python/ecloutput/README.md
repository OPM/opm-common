pybind11 linked to original repo 


cloning pybind11 => into '/home/tskille/prog/tmp/opm-common/python/pybind11'...

1) git submodule init
2) git submodule update



in subfolder    
  opm-common/python/ecloutput
  
3) mkdir build
4) cd build
5) cmake ..
6) make 


python package eclOutput is now ready to be used.


Remember to have 

   ~/opm-common/python/ecloutput/module in your $PYTHONPATH


Notice that the module can be re-locatable as long as the folder 'shared_obj_files' is located 
along side the folder 'module' 


example of usage:


 > python2.7 test_eminit.py 9_EDITNNC


script file: test_eminit.py

    import sys

    from eclOutput import EclModInit
    
    
    if __name__ == "__main__":
        
        init1 = EclModInit(sys.argv[1])
        
        init1.addFilter("EQLNUM", "eq", 2) 
        init1.addFilter("SWAT", "lt", 1.0) 
         
        print ("\nNumber of active cells in HC Zone     :  %i " % init1.getNumberOfActiveCells())
    
        celvol= init1.getParam("CELLVOL")
        
        print ("gross rock volume equlibrium region 1 : %7.2f M Rm3 \n" % float(sum(celvol)/1.0e6))
            
        print("Finished, all good")


