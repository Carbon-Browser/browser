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

if [ -z "$CI_MERGE_REQUEST_TARGET_BRANCH_NAME" ]; then
  echo "This is not a merge request, skipping lint"
  exit 0
fi

FORMATTING_ERROR_CODE=0


echo "Ensuring the merge base commit is in the (truncated) history"
if ! git merge-base --is-ancestor "$CI_MERGE_REQUEST_DIFF_BASE_SHA" HEAD; then
  echo "$CI_MERGE_REQUEST_DIFF_BASE_SHA not present in history. Unshallowing the git repo is slow and flaky (DPD-2687), so I am skipping the linting and formatting checks"
  exit 1
fi

echo "cpplint.py will run for cpp files modified against $CI_MERGE_REQUEST_TARGET_BRANCH_NAME"
FILES_TO_CHECK=$(git diff-tree --name-only -r $CI_MERGE_REQUEST_DIFF_BASE_SHA $CI_COMMIT_SHA -- '*.cc' '*.h' | grep -E 'adblock|eyeo' | grep -v 'schema_hash.h') || true
if [ ! -z "$FILES_TO_CHECK" ]; then
  echo "cpplint.py will check $(echo "$FILES_TO_CHECK" | wc -l) files"
  # Muted some lint checks which produce false positives (runtime/references) or lot of noise.
  LINT_OUTPUT=$(cpplint.py --filter=-whitespace,-build/include_what_you_use,-runtime/references,-readability/todo,-build/namespaces,-runtime/int $FILES_TO_CHECK 2>&1  > /dev/null | grep -v "Skipping input") || true
  echo "$LINT_OUTPUT"
  if [[ $LINT_OUTPUT != "Total errors found: 0" ]]; then
    # Set a lint error but continue with the script, maybe there are also formatting errors
    FORMATTING_ERROR_CODE=111
  else
    echo "No cpplint errors found"
  fi
else
  echo "No cpp files to lint in this MR"
fi

echo "Will run git cl format for all files modified against $CI_MERGE_REQUEST_TARGET_BRANCH_NAME"
FORMAT_DIFF=$(git cl format --diff --upstream=${CI_MERGE_REQUEST_DIFF_BASE_SHA} --python  2>&1)
if [[ -n "$FORMAT_DIFF" ]]; then
  echo "$FORMAT_DIFF";
  ((FORMATTING_ERROR_CODE+=222))
else
  echo "No git cl format errors found"
fi

exit $FORMATTING_ERROR_CODE
