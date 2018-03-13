#!/bin/bash

# Temporary backwards compat code
if test -n "$OPM_DATA_ROOT"
then
  OPM_TESTS_ROOT=$OPM_DATA_ROOT
fi
if test -n "$OPM_DATA_ROOT_PREDEFINED"
then
  OPM_TESTS_ROOT_PREDEFINED=$OPM_DATA_ROOT_PREDEFINED
fi

# Predefined by environment
if test -z "$OPM_TESTS_ROOT"
then
  OPM_TESTS_REVISION="master"
  if grep -q "opm-tests=" <<< $ghprbCommentBody
  then
    OPM_TESTS_REVISION=pull/`echo $ghprbCommentBody | sed -r 's/.*opm-tests=([0-9]+).*/\1/g'`/merge
  fi
  # Not specified in trigger, use shared copy
  if [[ "$OPM_TESTS_REVISION" = "master" ]] && [[ ! "$OPM_TESTS_ROOT_PREDEFINED" = "" ]]
  then
    if ! test -d $WORKSPACE/deps/opm-tests
    then
      cp $OPM_TESTS_ROOT_PREDEFINED $WORKSPACE/deps/opm-tests -R
    fi
  else
    # Specified in trigger, download it
    source $WORKSPACE/deps/opm-common/jenkins/build-opm-module.sh
    clone_module opm-tests $OPM_TESTS_REVISION
  fi
else
  if ! test -d $WORKSPACE/deps/opm-tests
  then
    cp $OPM_TESTS_ROOT $WORKSPACE/deps/opm-tests -R
  fi
fi
OPM_TESTS_ROOT=$WORKSPACE/deps/opm-tests
