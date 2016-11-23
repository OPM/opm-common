# Loading pycmake macros

python_module( ctypes )
python_module( six    )
python_module( numpy  REQUIRED 1.7  )
python_module( inspect    OPTIONAL  )

if (DEFINED PY_inspect)
  message(STATUS "We have inspect.")
else()
  message(STATUS "We do not have inspect, ignoring ...")
endif()
