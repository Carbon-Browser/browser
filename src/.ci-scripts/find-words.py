#!/bin/python

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

import argparse
import git
import re

searched_words = set()
# Bundled filter lists may contain searched words, skip them.
excluded_paths = ['.txt']
# Don't check same commits twice.
checked_commits = set()


def report_word_usage(checked_string, description):
  for word in searched_words:
    for match in re.findall(r'\b' + word + r'\b', checked_string, re.IGNORECASE):
      print(description + match)


def find_vanilla_commit(repo, head):
  # Assumption: vanilla commit which marks the beginning of eyeo Chromium SDK
  # changes on a branch is the first (latest) commit that has
  # 'Publish DEPS for ...' commit message.
  for commit in repo.iter_commits(head, grep='Publish DEPS for .*', max_count=1):
    return commit
  return None


def path_is_excluded(diff_item):
  for path in excluded_paths:
    if re.findall(path, diff_item.b_path):
      return True
  return False


def validate_diff(repo, commit):
  parent = commit.parents[0]
  for diff_item in commit.diff(parent).iter_change_type('M'):
    if diff_item.b_blob and not path_is_excluded(diff_item):
      printable_diff = repo.git.diff(
          str(commit), str(parent), '--', diff_item.b_path)
      report_word_usage(printable_diff, 'Diff of ' + str(commit) +
                        ' in file ' + diff_item.b_path + ' contains bad word ')


def validate_commit(repo, commit):
  report_word_usage(commit.message, 'Commit message ' +
                    str(commit) + ' contains bad word ')
  validate_diff(repo, commit)


def validate_history(repo, head):
  vanilla_commit = find_vanilla_commit(repo, head)
  if not vanilla_commit:
    return
  for commit in repo.iter_commits(str(vanilla_commit) + '..' + str(head), no_merges=True):
    if commit in checked_commits:
      return
    checked_commits.add(commit)
    validate_commit(repo, commit)


def validate_branch_name(ref):
  report_word_usage(ref.name, 'Branch ' + ref.name + ' contains bad word ')


parser = argparse.ArgumentParser(
    description='Find words in eyeo Chromium SDK repo. Search through branch names, commit messages and diffs')
parser.add_argument('words', metavar='WORDS', nargs='+',
                    help='space-separated words to search for, will be matched case-insensitive and as whole words')
args = parser.parse_args()
searched_words = args.words
repo = git.Repo('.')
for ref in repo.remote().refs:
  validate_branch_name(ref)
  validate_history(repo, ref.commit)
