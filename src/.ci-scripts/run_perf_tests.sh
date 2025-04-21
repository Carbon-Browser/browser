#!/bin/bash

# This file is part of eyeo Chromium SDK,
# Copyright (C) 2006-present eyeo GmbH
# eyeo Chromium SDK is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
# eyeo Chromium SDK is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.

# How many times we will repeat components_perftests
SAMPLES_COUNT=30
if [[ ! -z $1 ]]; then
  SAMPLES_COUNT=$1
fi

# Build for current branch
time autoninja -j${NUMJOBS} -C out/Release components_perftests adblock_flatbuffer_converter

# Store the size of flatbuffer files generated on the current branch:
cp .ci-scripts/print_size_of_flatbuffer.py .
exceptionrules_size_current=$(python3 print_size_of_flatbuffer.py out/Release/adblock_flatbuffer_converter components/test/data/adblock/exceptionrules.txt.gz)
easylist_size_current=$(python3 print_size_of_flatbuffer.py out/Release/adblock_flatbuffer_converter components/test/data/adblock/easylist.txt.gz)
echo "Size of current exceptionrules.fb: $exceptionrules_size_current bytes"
echo "Size of current easylist.fb: $easylist_size_current bytes"


# Run components_perftests for current branch
for i in $(seq 1 $SAMPLES_COUNT); do
  echo "Getting current sample $i"
  time xvfb-run -a ./out/Release/components_perftests --no-sandbox --gtest_filter="*Adblock*:*Eyeo*" > "components_perftests_current${i}.txt"
  cat "components_perftests_current${i}.txt" | sed -En "s/.*RESULT\ (.*)=\ ([0-9]*)\ (.*)/\1 (\3) \2/gp" > "components_perftests_current${i}_parsed.txt"
done

# We want to use compare script for current branch (it may have been updated in the MR)
cp .ci-scripts/compare_perf_samples.py .

# Checkout baseline branch (MR target)
git branch -D $CI_MERGE_REQUEST_TARGET_BRANCH_NAME || true
git fetch origin $CI_MERGE_REQUEST_TARGET_BRANCH_NAME --depth 1
git checkout origin/$CI_MERGE_REQUEST_TARGET_BRANCH_NAME

# Build for baseline branch
time autoninja -j${NUMJOBS} -C out/Release components_perftests adblock_flatbuffer_converter

# Store the size of flatbuffer files generated on the baseline branch:
exceptionrules_size_baseline=$(python3 print_size_of_flatbuffer.py out/Release/adblock_flatbuffer_converter components/test/data/adblock/exceptionrules.txt.gz)
easylist_size_baseline=$(python3 print_size_of_flatbuffer.py out/Release/adblock_flatbuffer_converter components/test/data/adblock/easylist.txt.gz)
echo "Size of baseline exceptionrules.fb: $exceptionrules_size_baseline bytes"
echo "Size of baseline easylist.fb: $easylist_size_baseline bytes"

# Run components_perftests for baseline branch
for i in $(seq 1 $SAMPLES_COUNT); do
  echo "Getting baseline sample $i";
  time xvfb-run -a ./out/Release/components_perftests --no-sandbox --gtest_filter="*Adblock*:*Eyeo*" > "components_perftests_baseline${i}.txt"
  cat "components_perftests_baseline${i}.txt" | sed -En "s/.*RESULT\ (.*)=\ ([0-9]*)\ (.*)/\1 (\3) \2/gp" > "components_perftests_baseline${i}_parsed.txt"
done

EXIT_CODE=0

# Every file from current branch must have the same number of entries
# Let's take the 1st one to see how many test results we will compare
echo "--- PERF TESTS COMPARING START ---"
NUMBER_OF_TESTS=$(cat components_perftests_current1_parsed.txt | wc -l)
for i in $(seq 1 $NUMBER_OF_TESTS); do
  CLINE=$(sed "${i}q;d" "components_perftests_current1_parsed.txt")
  CLABEL=$(echo "$CLINE" | sed -En "s/(.*)\ ([0-9]*)/\1/gp")
  if [[ "$i" -ne "1" ]]; then
    echo "-------------------------------------------"
  fi
  echo "Comparing measurements for \"$CLABEL\":"
  if [[ -z $(cat components_perftests_baseline1_parsed.txt | grep "$CLABEL") ]]; then
    echo "\"$CLABEL\" missing on baseline branch, moving to the next test."
    continue
  fi
  CURRENT=""
  BASELINE=""
  for j in $(seq 1 $SAMPLES_COUNT); do
    CLINE=$(sed "${i}q;d" "components_perftests_current${j}_parsed.txt")
    C=$(echo "$CLINE" | sed -En "s/(.*)\ ([0-9]*)/\2/gp")
    CURRENT="${CURRENT}${C},"
    BLINE=$(cat components_perftests_baseline${j}_parsed.txt | grep "$CLABEL")
    B=$(echo "$BLINE" | sed -En "s/(.*)\ ([0-9]*)/\2/gp")
    BASELINE="${BASELINE}${B},"
  done
  # Remove trailing comma
  CURRENT=${CURRENT::-1}
  BASELINE=${BASELINE::-1}
   # Run script which compares samples
  ./compare_perf_samples.py --current ${CURRENT} --baseline ${BASELINE}
  if [[ $? -ne 0 ]]; then
    EXIT_CODE=1
  fi
done

# Compare the size of flatbuffer files
echo "Comparing the size of flatbuffer files:"
if [[ $exceptionrules_size_current -gt $exceptionrules_size_baseline ]]; then
  echo "Size of exceptionrules.fb regressed from $exceptionrules_size_baseline to $exceptionrules_size_current bytes"
  EXIT_CODE=1
elif [[ $easylist_size_current -gt $easylist_size_baseline ]]; then
  echo "Size of easylist.fb regressed from $easylist_size_baseline to $easylist_size_current bytes"
  EXIT_CODE=1
else
  echo "Size of exceptionrules.fb and easylist.fb did not regress"
fi

echo "--- PERF TESTS COMPARING END ---"
exit $EXIT_CODE
