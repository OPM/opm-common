#!/bin/bash
set -e

rst_deck=$1
opmhash=$2
deck=$3

pushd $(mktemp -d)

${rst_deck} ${deck} HISTORY:0 output/CASE_COPY.DATA -m copy -s
${opmhash} ${deck} output/CASE_COPY.DATA -S

${rst_deck} ${deck} HISTORY:0 -m share -s > CASE_STDOUT.DATA
${opmhash} ${deck} CASE_STDOUT.DATA -S

pushd output
chmod -R a-w *
popd

${rst_deck} output/CASE_COPY.DATA HISTORY:0 output/CASE_SHARE.DATA -m copy -s
${opmhash} ${deck} output/CASE_SHARE.DATA -S
popd
