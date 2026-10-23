#!/bin/bash

# Handle run trigger, both runs use the default configuration,
# and the wells are plotted once after the runs
procs=""
if grep -qE " norne\>" <<< $ghprbCommentBody
then
  $WORKSPACE/deps/opm-simulators/jenkins/run-norne.sh default
  test $? -eq 0 || exit 1
  ran=1
fi
if grep -qE " norne_parallel\>" <<< $ghprbCommentBody
then
  $WORKSPACE/deps/opm-simulators/jenkins/run-norne.sh default 8
  test $? -eq 0 || exit 1
  procs=8
  ran=1
fi

if test -n "$ran"
then
  $WORKSPACE/deps/opm-simulators/jenkins/plot-norne.sh default $procs
fi
