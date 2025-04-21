#!/usr/bin/env python3

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

"""
Given the path to a test report, prints the list of failed tests to exclude,
in a format that can be consumed by a GTest command.

Example: --gtest_filter=-test1:test2:test3

In the case of no failed tests reported, a wilcard is printed instead.
Warning: This is valid according to GTest rules, but its use is discouraged
because it prevents parallel test execution.
"""

import argparse
import os
import sys


def get_gtest_selector(file):
    if os.path.isfile(file) and os.stat(file).st_size > 0:
        with open(file) as f:
            return "-" + ':'.join(line.strip() for line in f)
    return "*"

def main(args):
    try:
        tests = get_gtest_selector(args.test_report)
        print(tests)
    except FileNotFoundError as e:
        print(str(e), file=sys.stderr)
        exit(e.errno)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument(
        "test_report",
        type=str,
        help="The path to the test report to read failed tests from"
    )

    args = parser.parse_args()
    main(args)
