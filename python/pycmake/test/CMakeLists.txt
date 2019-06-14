# Test the module - assumes some packages are installed.

find_package( PythonInterp 2.6 REQUIRED )
include( ${CMAKE_SOURCE_DIR}/FindPythonModule.cmake )

python_module( ctypes )
python_module( numpy  REQUIRED 1.8 )
python_module( pandas REQUIRED 0.18.1 EXACT )

python_module( matplotlib REQUIRED 1.2    MINIMUM                    )
python_module( PyQt4      REQUIRED 4.8                               )
python_module( PyQt4.Qt   REQUIRED 4.8    MINIMUM   "QT_VERSION_STR" )
python_module( PyQt4.Qt   REQUIRED 4.10   MINIMUM "PYQT_VERSION_STR" )
python_module( re         REQUIRED 2.2.1  MINIMUM                    )
python_module( requests   REQUIRED 2.2.1  EXACT                      )
python_module( inspect    OPTIONAL                                   )
#python_module( pandas    REQUIRED 9.9.9  OPTIONAL )                 # will warn but pass

python_module( cwrap QUIET )
python_module( cwrap QUIET 1.0)
python_module( no_such_module QUIET )
python_module( scipy QUIET 0.16.1 )

if (DEFINED PY_inspect)
  # Here we can do stuff depending on having *inspect*
  message(STATUS "We have inspect.")
else()
  # And here we can do stuff when we do not have *inspect*
  message(STATUS "We do not have inspect, ignoring ...")
endif()

