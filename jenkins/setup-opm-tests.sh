#!/bin/bash

# Predefined by environment
if [[ -n $OPM_TESTS_ROOT ]] && grep -qiF 'opm-tests=' <<< "${ghprbCommentBody:-}"
then
  echo "Cannot use opm-tests= when OPM_TESTS_ROOT is predefined" >&2
  exit 1
fi

if test -z "$OPM_TESTS_ROOT"
then
  OPM_TESTS_REVISION="master"
  if [[ $ghprbCommentBody =~ (^|[[:space:]])opm-tests=([^[:space:]]*) ]]
  then
    requested_revision=${BASH_REMATCH[2]}
    remainder=${ghprbCommentBody#*"${BASH_REMATCH[0]}"}
    if grep -qiF 'opm-tests=' <<< "$remainder"
    then
      echo "Multiple opm-tests revisions specified" >&2
      exit 1
    fi
    if test -n "$absolute_revisions"
    then
      if [[ -z $requested_revision ]]
      then
        echo "Invalid opm-tests revision: a ref is required after opm-tests=" >&2
        exit 1
      fi
      if [[ $requested_revision == -* ]] || ! git check-ref-format --branch "$requested_revision" >/dev/null 2>&1
      then
        echo "Invalid opm-tests revision '$requested_revision': expected a branch, tag, or commit" >&2
        exit 1
      fi
      OPM_TESTS_REVISION=$requested_revision
    else
      if [[ ! $requested_revision =~ ^[1-9][0-9]*$ ]]
      then
        echo "Invalid opm-tests PR number '$requested_revision': expected a positive integer" >&2
        exit 1
      fi
      OPM_TESTS_REVISION=pull/$requested_revision/merge
    fi
  elif grep -qiF 'opm-tests=' <<< "${ghprbCommentBody:-}"
  then
    echo "Invalid opm-tests trigger: use opm-tests= as a standalone lowercase token" >&2
    exit 1
  fi
  # Not specified in trigger, use shared copy
  if [[ "$OPM_TESTS_REVISION" = "master" ]] && [[ ! "$OPM_TESTS_ROOT_PREDEFINED" = "" ]]
  then
    if ! test -d $WORKSPACE/deps/opm-tests
    then
      cp $OPM_TESTS_ROOT_PREDEFINED $WORKSPACE/deps/opm-tests -R
      pushd $WORKSPACE/deps/opm-tests
      echo "opm-tests revision: $(git rev-parse HEAD)"
      popd
    fi
  else
    # We need a full repo checkout
    if ! test -d "$WORKSPACE/deps/opm-tests"
    then
      cp "$OPM_TESTS_ROOT_PREDEFINED" "$WORKSPACE/deps/opm-tests" -R || exit 1
    fi
    pushd "$WORKSPACE/deps/opm-tests" || exit 1
    # Then we fetch the PR branch
    pr_remote=${OPM_TESTS_UPSTREAM:-https://github.com/OPM/opm-tests}
    if git remote get-url PR >/dev/null 2>&1
    then
      git remote set-url PR "$pr_remote" || exit 1
    else
      git remote add PR "$pr_remote" || exit 1
    fi
    if ! git fetch --depth 1 -- PR "$OPM_TESTS_REVISION"
    then
      echo "Failed to fetch opm-tests revision '$OPM_TESTS_REVISION'; check that the PR or ref exists" >&2
      exit 1
    fi
    if ! git checkout -B branch_to_build FETCH_HEAD
    then
      echo "Failed to check out opm-tests revision '$OPM_TESTS_REVISION'" >&2
      exit 1
    fi
    popd
  fi
else
  if ! test -d $WORKSPACE/deps/opm-tests
  then
    cp $OPM_TESTS_ROOT $WORKSPACE/deps/opm-tests -R
    pushd $WORKSPACE/deps/opm-tests
    echo "opm-tests-revision: $(git rev-parse HEAD)"
    popd
  fi
fi
OPM_TESTS_ROOT=$WORKSPACE/deps/opm-tests
