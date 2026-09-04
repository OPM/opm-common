if(dune-uggrid_FOUND)
  return()
endif()

if(dune-uggrid_FIND_REQUIRED)
  find_package(dune-uggrid CONFIG REQUIRED)
else()
  find_package(dune-uggrid CONFIG)
endif()

if(dune-uggrid_FOUND)
  target_compile_definitions(Dune::UGGrid INTERFACE HAVE_DUNE_UGGRID=1)
  # make version number available in config.h
  include (UseDuneVer)
  find_dune_version ("dune" "uggrid")
endif()
