#!/usr/bin/env bash
set -e

project_list=(opm-data opm-common opm-parser opm-material opm-core opm-output opm-grid opm-simulators opm-upscaling ewoms)

# Will clone all the projects *except* the one project given as commandline argument; that
# has typically been checked out by travis already.

function clone_project {
    url=https://github.com/OPM/${1}.git
    git clone $url
}


for project in "${project_list[@]}"; do
    if [ "$project" != $1 ]; then
        clone_project $project
    fi
done



