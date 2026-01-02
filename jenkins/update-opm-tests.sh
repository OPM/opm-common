#!/bin/bash

MAIN_REPO=$1 # The repo the update was triggered from

source $WORKSPACE/deps/opm-common/jenkins/build-opm-module.sh

declare -a upstreams # Everything is considered an upstream to aid code reuse
upstreams=(opm-common
           opm-grid
           opm-simulators
           opm-upscaling
          )

declare -A upstreamRev
upstreamRev[opm-common]=master
upstreamRev[opm-grid]=master
upstreamRev[opm-simulators]=master
upstreamRev[opm-upscaling]=master
upstreamRev[opm-tests]=master

# Setup revision tables
parseRevisions
upstreamRev[$MAIN_REPO]=$sha1

# Create branch name
BRANCH_NAME="update"
REASON="Unknown"
if test -z "$absolute_revisions"
then
  for repo in ${upstreams[*]}
  do
    if [ "${upstreamRev[$repo]}" != "master" ]
    then
      rev=${upstreamRev[$repo]}
      prnumber=${rev//[!0-9]/}
      BRANCH_NAME="${BRANCH_NAME}_${repo}_$prnumber"
      if [ "$REASON" = "Unknown" ]
      then
        REASON=""
      fi
      test -n "$REASON" && REASON+="        "
      REASON+="PR https://github.com/OPM/$repo/pull/$prnumber\n"
    fi
  done
fi

# Do the commit
export REASON
if [ "${upstreamRev[opm-tests]}" == "master" ]
then
  export BRANCH_BASE=origin/master
else
  export BRANCH_BASE=${upstreamRev[opm-tests]}
fi
export BRANCH_NAME
$WORKSPACE/deps/opm-simulators/tests/update_reference_data.sh $OPM_TESTS_ROOT $WORKSPACE/$configuration/build-opm-simulators $WORKSPACE/$configuration/install/bin/convertECL
if test $? -eq 5
then
  echo "No tests failed - no data to update. Exiting"
  exit 0
fi
