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
Runs Chromium tests (unit_tests, browser_tests etc.), optionally skipping tests that were deemed unstable in the past or updating the list of unstable tests.
"""

import argparse
import boto3
import logging
import os
import subprocess
import sys
import tempfile
from urllib.parse import urlparse
from pathlib import Path

import chromium_version
import s3_download_files
import s3_upload_files
import get_gtest_exclusion_filter
import get_failing_tests

s3 = boto3.client('s3')


# TODO(mpawlowski) move bucket_name and prefix to a shared file, to avoid duplication with update_flaky_tests.py
def bucket_name():
    return os.environ.get('AWS_S3_BUCKET_CENTRAL_1')


def get_prefix():
    version = chromium_version.get_chromium_version()
    # TODO(mpawlowski): We might want to store failures from different platforms in separate directories.
    # Currently, we're only collecting flakes from Desktop Linux.
    return f"builds-archive/chromium-{version}-vanilla-automated/vanilla_build_and_test_desktop_release"


def get_flake_list_name(test_binary_name, asan_enabled):
    return test_binary_name + ("_asan" if asan_enabled else "") + "_failed.txt"


def download_known_flakes_from_s3(test_binary_name, local_flake_list_path,
                                  asan_enabled):
    remote_flake_list_path = f"{get_prefix()}/{get_flake_list_name(test_binary_name, asan_enabled)}"
    # Download the flake list from S3. The list might not be there, which is fine.
    try:
        logging.debug(
            f"Downloading flaky tests list from {remote_flake_list_path}")
        s3_download_files.download_with_retry(
            bucket_name(), get_prefix(),
            get_flake_list_name(test_binary_name, asan_enabled),
            local_flake_list_path)
        logging.debug(f"List downloaded to {local_flake_list_path}")
    except s3.exceptions.ClientError:
        logging.debug(
            f"Flaky tests list not found at {remote_flake_list_path}")


def upload_backup_of_flake_list(test_binary_name, local_flake_list_path,
                                asan_enabled):
    remote_flake_list_backup_path = f"{get_prefix()}/{get_flake_list_name(test_binary_name, asan_enabled)}.bak"
    s3_upload_files.upload_files(
        local_flake_list_path, bucket_name(), get_prefix(),
        f"{get_flake_list_name(test_binary_name, asan_enabled)}.bak")
    logging.debug(f"Backup of the original flaky tests list uploaded to " +
                  f"{remote_flake_list_backup_path}")


def update_local_flake_list(local_flake_list_path, current_failing_tests):
    known_failing_tests = set()
    if os.path.exists(local_flake_list_path):
        with open(local_flake_list_path, "r") as f:
            known_failing_tests = set(f.read().splitlines())
    logging.info(
        f"There are {len(current_failing_tests - known_failing_tests)} new failing tests"
    )
    logging.debug(f"Currently known failing tests: {len(known_failing_tests)}")
    known_failing_tests.update(current_failing_tests)
    logging.debug(
        f"New number of known failing tests: {len(known_failing_tests)}")
    with open(local_flake_list_path, "w") as f:
        f.write("\n".join(known_failing_tests))


def upload_flaky_tests_list(test_binary_name, local_flake_list_path,
                            asan_enabled):
    remote_flake_list_path = f"{get_prefix()}/{get_flake_list_name(test_binary_name, asan_enabled)}"
    logging.debug(f"Uploading list of flaky tests to {remote_flake_list_path}")
    s3_upload_files.upload_files(
        local_flake_list_path, bucket_name(), get_prefix(),
        get_flake_list_name(test_binary_name, asan_enabled))
    logging.debug("Flaky tests list updated")


def run_tests(test_binary_name, out_dir, skipped_tests_path, extra_args):
    repo_root = Path(__file__).resolve().parent.parent.absolute()
    test_binary = repo_root / out_dir / "bin" / f"run_{test_binary_name}"
    if not test_binary.exists():
        raise FileNotFoundError(f"Test binary {test_binary} not found")
    # We report the result of the test run to a file with the same name as the test binary, but with a .xml extension
    report_file = repo_root / out_dir / f"{test_binary_name}_report.xml"
    command = [
        "time", "xvfb-run", "-s", "-screen 0 1024x768x24",
        str(test_binary), f"--gtest_output=xml:{report_file}"
    ] + extra_args

    selector = get_gtest_exclusion_filter.get_gtest_selector(
        skipped_tests_path)
    if selector != "*":
        command += ["--gtest_filter=" + selector]

    logging.debug(f"Running command: {' '.join(command)}")
    # Run the test whilst printing the output to the console as it happens. This
    # is useful for debugging test failures, especially if the job takes so long
    # that the command doesn't finish within the time limit.
    process = subprocess.Popen(command,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
    for line in iter(process.stdout.readline, b''):
        print(line.decode(), end='')
        sys.stdout.flush()

    process.stdout.close()
    return (process.wait(), report_file)


def main(args):
    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)
    s3 = boto3.client('s3')

    test_binary_name = args.test_binary_name
    with tempfile.TemporaryDirectory() as temp_dir:
        # Download the flake list from S3 when we want to skip known upstream failures or update the flake list
        if args.skip_known_upstream_failures or args.update_flake_list:
            download_known_flakes_from_s3(test_binary_name, temp_dir,
                                          args.asan)
        # Run the tests
        local_flake_list_path = os.path.join(
            temp_dir, get_flake_list_name(test_binary_name, args.asan))
        return_code, report_file = run_tests(test_binary_name, args.out_dir,
                                             local_flake_list_path,
                                             args.extra_args)

        # In case of errors, optionally update the flake list
        if return_code != 0 and args.update_flake_list:
            # Find the list of tests that failed, crashed or timed out.
            current_failing_tests = set(
                get_failing_tests.get_gtest_selector(report_file).split("\n"))

            # If there are too many failed tests, perhaps the whole run is compromised or the CI is broken.
            # In such case, we don't want to update the flake list.
            if len(current_failing_tests) > 500:
                logging.warning(
                    f"Too many tests failed ({len(current_failing_tests)}), not updating the flaky tests list."
                )
                return return_code

            # Before uploading the modified list, upload a backup of the original list
            if os.path.exists(local_flake_list_path):
                upload_backup_of_flake_list(test_binary_name,
                                            local_flake_list_path, args.asan)
            # Update the local flake list with new failures
            update_local_flake_list(local_flake_list_path,
                                    current_failing_tests)

            # Upload the updated list to S3
            upload_flaky_tests_list(test_binary_name, local_flake_list_path,
                                    args.asan)

            # When we're in flake-collection mode, we _expect_ run_tests() will return an error code,
            # so we don't want to return the return_code from run_tests() here. As long as the
            # update of the flake list was successful, we should return a success.
            return 0
        return return_code


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("out_dir",
                        type=str,
                        help="The directory where the test binary is located")
    parser.add_argument("test_binary_name",
                        type=str,
                        help="The name of the test binary to run")
    parser.add_argument("--skip_known_upstream_failures",
                        action='store_true',
                        help="Skip tests that are known to be flaky upstream")
    parser.add_argument("--update_flake_list",
                        action='store_true',
                        help="Update the list of flaky tests")
    parser.add_argument("-v",
                        "--verbose",
                        action='store_true',
                        help="Print debug information")
    parser.add_argument(
        "-a",
        "--asan",
        action='store_true',
        default=False,
        help="If true, list of flaky tests for builds with asan will be updated"
    )
    parser.add_argument("extra_args",
                        nargs=argparse.REMAINDER,
                        default=[],
                        help="Extra arguments to pass to the test binary")
    args = parser.parse_args()
    exit(main(args))
