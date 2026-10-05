#! /bin/bash

./bin/genEvalSpecializations.py

if test -n "$(git diff)"; then
    echo "The generated source files have been manually edited or the "
    echo "code generator has been modified but not been run before "
    echo "proposing the branch for merging."
    exit 1
fi

./bin/genPythonStubKeywords.py || exit 1

if test -n "$(git diff)"; then
    echo "The keyword properties in python/opm_embedded/__init__.pyi do not "
    echo "match the keyword list. Run ./bin/genPythonStubKeywords.py and "
    echo "commit the result."
    exit 1
fi

echo "The generated source files have not been manually edited."
exit 0
