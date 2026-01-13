#!/bin/bash

# Handle run trigger
if grep -qE " norne\>" <<< $ghprbCommentBody
then
  if grep -q "serial" <<< $ghprbCommentBody
  then
    $WORKSPACE/deps/opm-simulators/jenkins/run-norne.sh serial
  else
    $WORKSPACE/deps/opm-simulators/jenkins/run-norne.sh default
  fi
  test $? -eq 0 || exit 1
fi
if grep -qE " norne_parallel\>" <<< $ghprbCommentBody
then
  $WORKSPACE/deps/opm-simulators/jenkins/run-norne.sh default 8
  test $? -eq 0 || exit 1
fi
