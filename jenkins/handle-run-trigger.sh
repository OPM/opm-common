#!/bin/bash

# Handle run trigger
if grep -q " norne" <<< $ghprbCommentBody
then
  if grep -q "serial" <<< $ghprbCommentBody
  then
    $WORKSPACE/deps/opm-simulators/jenkins/run-norne.sh serial
  else
    $WORKSPACE/deps/opm-simulators/jenkins/run-norne.sh mpi
  fi
  test $? -eq 0 || exit 1
fi
if grep -q " norne_parallel" <<< $ghprbCommentBody
then
  $WORKSPACE/deps/opm-simulators/jenkins/run-norne.sh mpi 4
  test $? -eq 0 || exit 1
fi
