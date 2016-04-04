#!/bin/bash

# $1 = Additional cmake parameters
# $2 = 0 to build and install module, 1 to build and test module
# $3 = Source root of module to build
function build_module {
  cmake $3 -DCMAKE_BUILD_TYPE=Release $1
  test $? -eq 0 || exit 1
  if test $2 -eq 1
  then
    make test-suite
    test $? -eq 0 || exit 2
    ctest -T Test --no-compress-output
    $WORKSPACE/deps/opm-common/jenkins/convert.py -x $WORKSPACE/deps/opm-common/jenkins/conv.xsl -t . > testoutput.xml
  else
    make install
  fi
}

# $1 = Name of module
# $2 = git-rev to use for module
function clone_module {
  pushd .
  mkdir -p $WORKSPACE/deps/$1
  cd $WORKSPACE/deps/$1
  git init .
  git remote add origin https://github.com/OPM/$1
  git fetch origin $2:branch_to_build
  git checkout branch_to_build
  test $? - eq 0 || exit 1
  popd
}

# $1 = Module to clone
# $2 = Additional cmake parameters
# $3 = git-rev to use for module
# $4 = Build root
function clone_and_build_module {
  clone_module $1 $3
  pushd .
  mkdir $4/build-$1
  cd $4/build-$1
  build_module $2 0 $WORKSPACE/deps/$1
  test $? -eq 0 || exit 1
  popd
}
