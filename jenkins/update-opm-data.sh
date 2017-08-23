#!/bin/bash

MAIN_REPO=$1 # The repo the update was triggered from

source $WORKSPACE/deps/opm-common/jenkins/build-opm-module.sh

declare -a upstreams # Everything is considered an upstream to aid code reuse
upstreams=(libecl
           opm-common
           opm-parser
           opm-output
           opm-material
           opm-grid
           opm-core
           ewoms
           opm-simulators
           opm-upscaling
          )

declare -A upstreamRev
upstreamRev[libecl]=master
upstreamRev[opm-common]=master
upstreamRev[opm-parser]=master
upstreamRev[opm-material]=master
upstreamRev[opm-core]=master
upstreamRev[opm-grid]=master
upstreamRev[opm-output]=master
upstreamRev[ewoms]=master
upstreamRev[opm-simulators]=master
upstreamRev[opm-upscaling]=master

# Setup revision tables
parseRevisions
upstreamRev[$MAIN_REPO]=$sha1

# Create branch name
BRANCH_NAME="update"
for repo in ${upstreams[*]}
do
  if [ "${upstreamRev[$repo]}" != "master" ]
  then
    rev=${upstreamRev[$repo]}
    prnumber=${rev//[!0-9]/}
    BRANCH_NAME="${BRANCH_NAME}_${repo}_$prnumber"
    test -n "$REASON" && REASON+="        "
    REASON+="https://github.com/OPM/$repo/pull/$prnumber\n"
  fi
done

# Do the commit
export REASON
export BRANCH_NAME
$WORKSPACE/deps/opm-simulators/tests/update_reference_data.sh $OPM_DATA_ROOT

sleep 5

# Finally open the pull request
cd $OPM_DATA_ROOT
git remote add jenkins4opm git@github.com:jenkins4opm/opm-data
git-open-pull -u jenkins4opm --base-account OPM --base-repo opm-data -r /tmp/cmsg $BRANCH_NAME
