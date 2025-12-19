#!/bin/bash

if test -z "$BTYPES"
then
  source $TOOLCHAIN_DIR/build-configurations.sh
fi

declare -A configurations

declare -A EXTRA_MODULE_FLAGS
EXTRA_MODULE_FLAGS[opm-simulators]="-DBUILD_FLOW_VARIANTS=ON -DOPM_ENABLE_PYTHON=ON -DBUILD_FLOW_POLY_GRID=ON -DBUILD_FLOW_ALU_GRID=ON"
EXTRA_MODULE_FLAGS[opm-common]="-DOPM_ENABLE_PYTHON=ON -DOPM_ENABLE_EMBEDDED_PYTHON=ON -DOPM_INSTALL_PYTHON=ON"

# Parse revisions from trigger comment and setup arrays
# Depends on: 'upstreams', upstreamRev',
#             'downstreams', 'downstreamRev',
#             'ghprbCommentBody',
#             'CONFIGURATIONS', 'TOOLCHAINS'
function parseRevisions {
  for upstream in ${upstreams[*]}
  do
    if grep -qi "$upstream=" <<< $ghprbCommentBody
    then
      if test -n "$absolute_revisions"
      then
        upstreamRev[$upstream]=`echo $ghprbCommentBody | sed -r "s/.*${upstream,,}=([^ ]+).*/\1/g"`
      else
        upstreamRev[$upstream]=pull/`echo $ghprbCommentBody | sed -r "s/.*${upstream,,}=([0-9]+).*/\1/g"`/merge
      fi
    fi
  done
  if grep -q "with downstreams" <<< $ghprbCommentBody
  then
    for downstream in ${downstreams[*]}
    do
      if grep -qi "$downstream=" <<< $ghprbCommentBody
      then
        if test -n "$absolute_revisions"
        then
          downstreamRev[$downstream]=`echo $ghprbCommentBody | sed -r "s/.*${downstream,,}=([^ ]+).*/\1/g"`
        else
          downstreamRev[$downstream]=pull/`echo $ghprbCommentBody | sed -r "s/.*${downstream,,}=([0-9]+).*/\1/g"`/merge
       fi
     fi
    done
  fi

  # Default to a serial build if no types are given
  if test -z "$BTYPES"
  then
    BTYPES="serial"
  fi

  # Convert to arrays for easy looping
  declare -a BTYPES_ARRAY
  for btype in $BTYPES
  do
    BTYPES_ARRAY=(${BTYPES_ARRAY[*]} $btype)
  done
  TOOLCHAIN_ARRAY=($CMAKE_TOOLCHAIN_FILES)
  for index in ${!BTYPES_ARRAY[*]}
  do
    key=${BTYPES_ARRAY[$index]}
    data=${TOOLCHAIN_ARRAY[$index]}
    configurations[$key]=$data
  done
}

# Print revisions and configurations
# $1 = Name of main module
# Depends on: 'upstreams', upstreamRev',
#             'downstreams', 'downstreamRev',
#             'ghprbCommentBody',
#             'configurations', 'sha1'
function printHeader {
  echo -e "Repository revisions:"
  for upstream in ${upstreams[*]}
  do
    echo -e "\t   [upstream] $upstream=${upstreamRev[$upstream]}"
  done
  echo -e "\t[main module] $1=$sha1"
  if grep -q "with downstreams" <<< $ghprbCommentBody
  then
    for downstream in ${downstreams[*]}
    do
      echo -e "\t [downstream] $downstream=${downstreamRev[$downstream]}"
    done
  fi

  echo "Configurations to process:"
  for conf in ${!configurations[@]}
  do
    echo -e "\t$conf=${configurations[$conf]}"
  done
}

# $1 = Additional cmake parameters
# $2 = 0 to build and install module, 1 to build and test module
# $3 = Source root of module to build
function build_module {
  CMAKE_PARAMS="$1"
  DO_TEST_FLAG="$2"
  MOD_SRC_DIR="$3"
  cmake "$MOD_SRC_DIR" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=$DO_TEST_FLAG -DCMAKE_TOOLCHAIN_FILE=${configurations[$configuration]} -DOPM_DOXYGEN_LOG_FILE=$PWD/doc/doxygen/Warnings.log $CMAKE_PARAMS
  test $? -eq 0 || exit 1
  if test $DO_TEST_FLAG -eq 1
  then

    pushd "$CWD"
    cd "$MOD_SRC_DIR"
    if test -x "./jenkins/pre-build.sh"; then
        echo "Running pre-build script"
        if ! "./jenkins/pre-build.sh"; then
            exit 1
        fi
    else
        echo "No pre-build script detected"
    fi
    popd

    if [ ! -z $BUILDTHREADS ]
    then
      cmake --build . -- -j$BUILDTHREADS
    else
      cmake --build .
    fi
    test $? -eq 0 || exit 2
    TESTTHREADS=${TESTTHREADS:-1}
    if test -n "${CTEST_TIMEOUT}"
    then
      TIMEOUT="--timeout ${CTEST_TIMEOUT}"
    else
      TIMEOUT=""
    fi

    ctest -T Test --no-compress-output -j$TESTTHREADS -LE "gpu_.*" ${TIMEOUT}

    # Convert to junit format
    $WORKSPACE/deps/opm-common/jenkins/convert.py -x $WORKSPACE/deps/opm-common/jenkins/conv.xsl -t . > testoutput.xml

    if ! grep -q "with downstreams" <<< $ghprbCommentBody
    then
      # Add configuration name
      sed -e "s/classname=\"TestSuite\"/classname=\"${configuration}\"/g" testoutput.xml > $WORKSPACE/$configuration/testoutput.xml
    fi
  else
    if [ ! -z $BUILDTHREADS ]
    then
      cmake --build . --target install -- -j$BUILDTHREADS
    else
      cmake --build . --target install
    fi
    test $? -eq 0 || exit 3
  fi
}

# $1 = Name of module
# $2 = git-rev to use for module
function clone_module {
  # Already cloned by an earlier configuration
  test -d $WORKSPACE/deps/$1 && return 0
  local repo_root=${OPM_REPO_ROOT:-https://github.com/OPM}
  mkdir -p $WORKSPACE/deps/$1
  pushd $WORKSPACE/deps/$1
  git init .
  git remote add origin ${repo_root}/$1
  git fetch --depth 1 origin $2:branch_to_build
  git checkout branch_to_build
  git log HEAD -1 | cat
  test $? -eq 0 || exit 1
  popd
}

# $1 = Name of module
# $2 = Additional cmake parameters
# $3 = Build root
# $4 = 1 to run tests
function init_and_build_module {
  mkdir -p $3/build-$1
  pushd $3/build-$1
  test_build=0
  if test -n "$4"
  then
    test_build=$4
  fi
  build_module "$2" $test_build $WORKSPACE/deps/$1
  test $? -eq 0 || exit 1
  popd
}

# Uses pre-filled arrays upstreams, and associativ array upstreamRev
# which holds the revisions to use for upstreams.
function build_upstreams {
  for upstream in ${upstreams[*]}
  do
    echo "Building upstream $upstream=${upstreamRev[$upstream]} configuration=$configuration"
    # Build upstream and execute installation
    init_and_build_module $upstream "-DCMAKE_PREFIX_PATH=$WORKSPACE/$configuration/install -DCMAKE_INSTALL_PREFIX=$WORKSPACE/$configuration/install ${EXTRA_MODULE_FLAGS[$upstream]}" $WORKSPACE/$configuration
    test $? -eq 0 || exit 1
  done
  test $? -eq 0 || exit 1
}

# $1 - name of the module we are called from
# Uses pre-filled arrays downstreams, and associativ array downstreamRev
# which holds the default revisions to use for downstreams
function build_downstreams {
  egrep_cmd="xml_grep --wrap testsuites --cond testsuite $WORKSPACE/$configuration/build-$1/testoutput.xml"
  for downstream in ${downstreams[*]}
  do
    echo "Building downstream $downstream=${downstreamRev[$downstream]} configuration=$configuration"
    # Build downstream and execute installation
    init_and_build_module $downstream "-DCMAKE_PREFIX_PATH=$WORKSPACE/$configuration/install -DCMAKE_INSTALL_PREFIX=$WORKSPACE/$configuration/install -DOPM_TESTS_ROOT=$OPM_TESTS_ROOT ${EXTRA_MODULE_FLAGS[$downstream]}" $WORKSPACE/$configuration 1
    test $? -eq 0 || exit 1

    # Installation for downstream
    pushd .
    cd $WORKSPACE/$configuration/build-$downstream

    if [ ! -z $BUILDTHREADS ]
    then
      cmake --build . --target install -- -j$BUILDTHREADS
    else
      cmake --build . --target install
    fi
    popd
    egrep_cmd="$egrep_cmd $WORKSPACE/$configuration/build-$downstream/testoutput.xml"
  done

  $egrep_cmd > $WORKSPACE/$configuration/testoutput.xml

  # Add testsuite name
  sed -e "s/classname=\"TestSuite\"/classname=\"${configuration}\"/g" -i $WORKSPACE/$configuration/testoutput.xml

  test $? -eq 0 || exit 1
}

# $1 = Name of main module
function clone_repositories {
  mkdir -p $WORKSPACE/deps
  for upstream in ${upstreams[*]}
  do
    if test "$upstream" != "opm-common"
    then
      clone_module ${upstream} ${upstreamRev[$upstream]}
    fi
  done

  if grep -q "with downstreams" <<< $ghprbCommentBody
  then
    for downstream in ${downstreams[*]}
    do
      clone_module ${downstream} ${downstreamRev[$downstream]}
    done
  fi

  ln -sf $WORKSPACE $WORKSPACE/deps/$1
}

# $1 = Name of main module
function build_module_full {
  PY_MAJOR=`python3 --version | awk -F ' ' '{print $2}' | awk -F '.' '{print $1}'`
  PY_MINOR=`python3 --version | awk -F ' ' '{print $2}' | awk -F '.' '{print $2}'`
  for configuration in ${!configurations[@]}
  do
    export PYTHONPATH="$WORKSPACE/$configuration/install/lib/python$PY_MAJOR.$PY_MINOR/site-packages"

    # Build upstream modules
    build_upstreams

    # Build main module
    mkdir -p $WORKSPACE/$configuration/build-$1
    pushd $WORKSPACE/$configuration/build-$1
    echo "Building main module $1=$sha1 configuration=$configuration"
    build_module "-DCMAKE_INSTALL_PREFIX=$WORKSPACE/$configuration/install -DOPM_TESTS_ROOT=$OPM_TESTS_ROOT ${EXTRA_MODULE_FLAGS[$1]}" 1 $WORKSPACE
    test $? -eq 0 || exit 1
    cmake --build . --target install
    test $? -eq 0 || exit 1
    popd

    # If no downstream builds we are done
    if grep -q "with downstreams" <<< $ghprbCommentBody
    then
      build_downstreams $1
      test $? -eq 0 || exit 1
    fi
  done

  # Optionally generate a failure report.
  # The report is always generated from the 'default' configuration data
  # since that is the configuration we have reference data for.
  if grep -q "failure_report" <<< $ghprbCommentBody
  then
    $WORKSPACE/deps/opm-simulators/tests/make_failure_report.sh \
    $WORKSPACE/deps/opm-tests \
    $WORKSPACE/default/build-opm-simulators \
    $WORKSPACE/default/build-opm-simulators
    test $? -eq 0 || exit 1
  fi

  if grep -q -i " run " <<< $ghprbCommentBody
  then
    $WORKSPACE/deps/opm-common/jenkins/handle-run-trigger.sh
    test $? -eq 0 || exit 1
  fi

  if grep -q -i "update_data" <<< $ghprbCommentBody
  then
    export configuration=default
    export OPM_TESTS_ROOT=$WORKSPACE/deps/opm-tests
    $WORKSPACE/deps/opm-common/jenkins/update-opm-tests.sh $1
    test $? -eq 0 || exit 1
  fi
}

# Run static analysis checks for a given module
# Assumes coverage_gcov, debug_iterator and clang_lto build configurations
# has been built.
# $1 - Name of main module
function run_static_analysis {
  if test -d $WORKSPACE/coverage_gcov
  then
    pushd $WORKSPACE/coverage_gcov/build-$1
    gcovr -j$TESTTHREADS --xml -r /build -o /build/coverage.xml
    popd
  fi

  if test -d $WORKSPACE/debug_iterator
  then
    pushd $WORKSPACE/debug_iterator/build-$1
    ninja doc
    popd
  fi

  COMPILE_COMMANDS=$WORKSPACE/clang_lto/build-$1/compile_commands.json
  if test -d $WORKSPACE/clang_lto
  then
    pushd $WORKSPACE
    infer run \
          --compilation-database $COMPILE_COMMANDS \
          --keep-going \
          --pmd-xml \
          -j $TESTTHREADS
    popd

    run-clang-tidy -checks='clang-analyzer.*' \
                   -p $WORKSPACE/clang_lto/build-$1/ \
                   -j$TESTTHREADS | tee $WORKSPACE/clang-tidy-report.log

    cppcheck --project=${COMPILE_COMMANDS} \
             -q \
              --force \
              --enable=all \
              --xml \
              --xml-version=2 \
              -i$WORKSPACE/tests/material/test_fluidmatrixinteractions.cpp \
              -j$TESTTHREADS \
              --output-file=$WORKSPACE/cppcheck-result.xml
  fi
}
