# Implement a new functional relationship between PERM and PORO

For normal opm/flow simulations the permeability is loaded from the input file
with the `PERMX` keyword. In this small exercise you will instead load two
parameters and then use a functional relationship between porosity and
permeability:

    permx = PERM_MIN + poro^POWER
    
The porosity field is loaded from the input file, in addition you should load
the PERM_MIN and POWER parameters from the input file. To solve this task you
must first make changes in opm-common to tell the parser about the new keyword,
and then subsequently make changes to opm-simulators to actually use the new
keyword.


## Add a new keyword in the opm-common repository

All the keywords recognized by the parser in opm-common are described as json
files, based on these files C++ code is generated in the build process. All the
keywords are in directory rooted at `src/opm/parser/eclipse/share/keywords`, in
addition the keywords must be listed in the
`src/opm/parser/eclipse/share/keywords/keyword_list.cmake`. For an example of
similar keyword look at `P/PINCH`

## Use the new keyword in opm-simulators

In the file `opm-simulators/opm/ebos/ecltransmissibility.hh` there is a function
`extractPermeability_()` which fetches the permeability from the opm-common
input layer. Modify this function to use the new `PERMFUN` keyword and calculate
the permeability instead of using the permeability entered in the input file.

Hint: to get access to the `PERM_MIN` and `POWER` values from the input file you
must used the `Deck` datastructure:

     const auto& deck = vanguard_.deck();
     const auto& permfun_kw = deck.getKeyword("PERMFUN");
     double perm_min = permfun_kw.getRecord(0).getItem("PERM_MIN").getSIDouble(0);
     double power = permfun_kw.getRecord(0).getItem("POWER").getSIDouble(0);
     
