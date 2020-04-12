#!/bin/bash

INPUT_CASE="$1"
SIMULATOR="$2"
EXIT_STATUS="$3"

work_dir=$(mktemp -d)
cp $INPUT_CASE ${work_dir}
pushd $work_dir

ecode=0
${SIMULATOR} ${INPUT_CASE}

sim_status=$?
if [ ${sim_status} -ne ${EXIT_STATUS} ]
then
   ecode=1
   echo "Test of exit status failed - expected: ${EXIT_STATUS} got ${sim_status}"
fi

popd
rm -rf ${work_dir}
exit $ecode
