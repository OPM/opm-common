# Changelog

A short month-by-month synopsis of change highlights. Most bugfixes won't make
it in here, only the bigger features and interface changes.

# 2016.11
* A new class, Runspec, for the RUNSPEC section, has been introduced
* Nodes in the FIELD group are no longer added to the Summary config
* WCONHIST only adds phases present in the deck
* cJSON can now be installed externally
* DeckItem and ParserItem internals refactored
* Build time reduced by only giving necessary source files to the json compiler
* Support for OPERATE, WSEGITER and GCONPROD
* Internal shared_ptrs removed from Schedule and children; interface updated
* Schedule is now copyable with regular C++ copy semantics - no internal refs
* Well head I/J is now time step dependent
* Well reference depth is time step dependent
* Some ZCORN issues fixed
* gas/oil and oil/gas ratio unit fixed for FIELD units

# 2016.10
* Significant improvements in overall parser performance
* shared_ptr has largely been removed from all public interfaces
* JFUNC keyword can be parsed
* Boolean conversions are explicit
* The Units.hpp header from core is moved here, replacing ConversionFactors
* The ConstPtr and Ptr shared pointer aliases are removed
* UnitSystem, Eclipse3DProperties, and OilVaporizationProperties are default
  constructible
