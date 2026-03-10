# for CMake >= 3.0, we need to change a few policies:
#
#   - CMP0026 to allow access to the LOCATION target property
#   - CMP0048 to indicate that we want to deal with the *VERSION*
#     variables ourselves
#   - CMP0064 to indicate that we want TEST if conditions to be evaluated
#   - CMP0074 to indicate that <PackageName>_ROOT can be used to find package
#             config files
macro(OpmSetPolicies)
  if (POLICY CMP0048)
    # We do not set version. Hence NEW should work and this can be removed later
    cmake_policy(SET CMP0048 NEW)
  endif()

  if(POLICY CMP0064)
    cmake_policy(SET CMP0064 NEW)
  endif()

  # set the behavior of the policy 0054 to NEW. (i.e. do not implicitly
  # expand variables in if statements)
  if (POLICY CMP0054)
    cmake_policy(SET CMP0054 NEW)
  endif()

  # set the behavior of policy 0074 to new as we always used <PackageName>_ROOT as the
  # root of the installation
  if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
  endif()

  # de-duplicates libraries for capable linkers (Apple ld64, lld, MSVC)
  if(POLICY CMP0156)
    cmake_policy(SET CMP0156 NEW)
  endif()

  # use boost config mode
  if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
  endif()
endmacro()
