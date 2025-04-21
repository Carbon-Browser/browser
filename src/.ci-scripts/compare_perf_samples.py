#!/usr/bin/env python3

# This file is part of eyeo Chromium SDK,
# Copyright (C) 2006-present eyeo GmbH
# eyeo Chromium SDK is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
# eyeo Chromium SDK is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with eyeo Chromium SDK.    If not, see <http://www.gnu.org/licenses/>.

import argparse
import scipy.stats as stats
import sys

def delta(current, baseline):
    try:
        return round((abs(current - baseline) / baseline) * 100.0, 2)
    except ZeroDivisionError:
        return 0

def average(list):
    return sum(list) / len(list)

def compare(args):
    current_sample = list(map(int, args.current.split(",")))
    baseline_sample = list(map(int, args.baseline.split(",")))

    # Perform t-test
    t_stat, p_value = stats.ttest_rel(baseline_sample, current_sample)
    avg_current_sample = average(current_sample)
    avg_baseline_sample = average(baseline_sample)
    avg_delta = delta(avg_current_sample, avg_baseline_sample)

    # Check p-value against significance level and check average delta against threshold (2%)
    alpha = 0.05
    avg_delta_threshold = 3
    if ((p_value < alpha) and (avg_delta > avg_delta_threshold)):
        print(f"Suspicious change detected. Current avg is {avg_current_sample}, baseline avg is {avg_baseline_sample}, both differs by {avg_delta}%")
        # Sort lists to better see the values
        current_sample.sort()
        baseline_sample.sort()
        print('current sample:', current_sample)
        print('baseline sample:', baseline_sample)
        exit(1)
    else:
        print('No significant changes.')
        exit(0)

parser = argparse.ArgumentParser(
    description='Compare samples from performance test results. Returns non zero when delta is statistically significant.'
)

parser.add_argument(
    "--current",
    required=True,
    type=str,
    help='Comma separated numbers (int) of current measurements, example "1,2,3"'
)

parser.add_argument(
    "--baseline",
    required=True,
    type=str,
    help='Comma separated numbers (int) of baseline measurements, example "1,2,3"'
)

args = parser.parse_args()
compare(args)
