#!/bin/bash

# This script performs a single analysis using cppcheck
# It is used by the 'make test' target in the buildsystems
# Usually you should use 'ctest -C cppcheck' rather than calling this script directly
#
# Parameters: $1 = Application binary
#             $2 = Source file to process
#             $3..$N = include path parameters (-I dir1 -I dir2 ...)

cppcheck_cmd=$1
source_file=$2
shift 2

tmpfil=$(mktemp)
$cppcheck_cmd $@ --inline-suppr --enable=all --suppress=unusedFunction $source_file &> $tmpfil

declare -a ignorere
ignorere=(\\[/usr.*\\]
          \\[\\*\\]
          \\[.*Too.many.\#ifdef)

nmatch=$(cat $tmpfil | grep "\[.*\]" | wc -l)
for RE in ${ignorere[*]}
do
  nign=$(cat $tmpfil | grep "$RE" | wc -l)
  let "nmatch=$nmatch-$nign"
done
cat $tmpfil
rm $tmpfil
test $nmatch -eq 0 || exit 1
