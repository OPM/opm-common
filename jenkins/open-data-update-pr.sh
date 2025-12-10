#!/bin/bash

MAIN_REPO=$1 # The repo the update was triggered from

# Open the pull request
cd $WORKSPACE/deps/opm-tests
git remote add jenkins4opm git@github.com:jenkins4opm/opm-tests
BRANCH_NAME=`git branch --sort=committerdate | tail -n1`

# Do some cleaning of old remote branches
# Easier code with git 2.7+
#BRANCHES=`git branch --sort=committerdate -r | grep jenkins4opm`
#NBRANCHES=`git branch --sort=committerdate -r | grep jenkins4opm | wc -l`
git fetch jenkins4opm # Sadly necessary with older git
BRANCHES=`git for-each-ref --sort=committerdate refs/remotes/jenkins4opm/ --format='%(refname:short)'`
NBRANCHES=`git for-each-ref --sort=committerdate refs/remotes/jenkins4opm/ --format='%(refname:short)' | wc -l`
if test $NBRANCHES -gt 30
then
  for BRANCH in $BRANCHES
  do
    BNAME=`echo $BRANCH | cut -f1 -d '/' --complement`
    if [ "$BNAME" != "HEAD" ]
    then
      git push jenkins4opm :$BNAME
    fi
    NBRANCHES=$((NBRANCHES-1))
    if test $NBRANCHES -lt 30
    then
      break
    fi
  done
fi

if [ -n "`echo $BRANCHES | tr ' ' '\n' | grep ^jenkins4opm/$BRANCH_NAME$`" ]
then
  GH_TOKEN=`git config --get gitOpenPull.Token`
  REV=${sha1}
  PRNUMBER=${rev//[!0-9]/}
  DATA_PR=`curl -X GET https://api.github.com/repos/OPM/opm-tests/pulls?head=jenkins4opm:$BRANCH_NAME | grep '"number":' | awk -F ':' '{print $2}' | sed -e 's/,//' -e 's/ //'`
  git push -u jenkins4opm $BRANCH_NAME -f
fi

if [ -n "$DATA_PR" ]
then
  curl -d "{ \"body\": \"Existing PR https://github.com/OPM/opm-tests/pull/$DATA_PR was updated\" }" -H "Authorization: token ${GH_TOKEN}" -X POST https://api.github.com/repos/OPM/$MAIN_REPO/issues/$PRNUMBER/comments
else
  $WORKSPACE/deps/opm-common/jenkins/git-open-pull -u jenkins4opm --base-account OPM --base-repo opm-tests -r /tmp/cmsg $BRANCH_NAME
fi
