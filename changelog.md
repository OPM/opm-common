# Changelog

A short month-by-month synopsis of change highlights. Most bugfixes won't make
it in here, only the bigger features and interface changes.

# 2016.10
* Significant improvements in overall parser performance
* shared_ptr has largely been removed from all public interfaces
* JFUNC keyword can be parsed
* Boolean conversions are explicit
* The Units.hpp header from core is moved here, replacing ConversionFactors
* The ConstPtr and Ptr shared pointer aliases are removed
* UnitSystem, Eclipse3DProperties, and OilVaporizationProperties are default
  constructible
