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
Downloads the list of flaky or failing upstream tests from the vanilla branch
relevant for the current checkout, allows updating the list by adding new flakes
and then uploads the updated list to S3.
"""

import argparse
import os
import tempfile
import boto3
from urllib.parse import urlparse

import chromium_version
import s3_download_files
import s3_upload_files

known_flake_lists = [
    "unit_tests_failed.txt",
    "components_unittests_failed.txt",
    "browser_tests_failed.txt",
]


def bucket_name():
    return os.environ.get('AWS_S3_BUCKET_CENTRAL_1')


def get_prefix():
    version = chromium_version.get_chromium_version()
    return f"builds-archive/chromium-{version}-vanilla-automated/vanilla_build_and_test_desktop_release"


def main(args):
    s3 = boto3.client('s3')
    flake_list = args.flake_list

    with tempfile.TemporaryDirectory() as temp_dir:
        remote_flake_list_path = f"{get_prefix()}/{flake_list}"
        local_flake_list_path = os.path.join(temp_dir, flake_list)
        # Download the flake list from S3
        s3_download_files.download_with_retry(bucket_name(),
                                              remote_flake_list_path,
                                              local_flake_list_path)

        # Read flakes from the list
        with open(local_flake_list_path, "r") as f:
            flaky_tests = f.read().splitlines()

        initial_flake_count = len(flaky_tests)

        # If there are no flakes to add or remove, just print the current list
        if not args.add and not args.remove:
            print(f"Current flaky tests list ({initial_flake_count}):")
            for t in flaky_tests:
                print(t)
            return

        # Add or remove flakes
        for t in args.add:
            if t not in flaky_tests:
                flaky_tests.append(t)
                print(f"Will add {t} to the flaky tests list")
            else:
                print(f"{t} is already in the flaky tests list")

        for t in args.remove:
            if t in flaky_tests:
                flaky_tests.remove(t)
                print(f"Will remove {t} from the flaky tests list")
            else:
                print(f"{t} is not in the flaky tests list")

        flaky_tests = list(set(flaky_tests))
        flaky_tests.sort()

        inp = input(
            f"Updated flaky tests list will have {len(flaky_tests)} entries, "
            + f"before it had {initial_flake_count}. Continue? y/N ")
        if inp.lower() != "y":
            print("Aborted")
            return

        # Before uploading the modified list, make a backup of the original list
        remote_flake_list_backup_path = f"{get_prefix()}/{flake_list}.bak"
        s3_upload_files.upload_files(local_flake_list_path, bucket_name(),
                                     get_prefix(), f"{flake_list}.bak")
        print(f"Backup of the original flaky tests list uploaded to " +
              f"{remote_flake_list_backup_path}")

        # Write the updated list
        with open(local_flake_list_path, "w") as f:
            f.write("\n".join(flaky_tests))

        # Upload the updated list to S3
        s3_upload_files.upload_files(local_flake_list_path, bucket_name(),
                                     get_prefix(), f"{flake_list}.bak")

        print(f"Updated flaky tests list uploaded to {remote_flake_list_path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument(
        "flake_list",
        type=str,
        help="The name of the flake list to update",
        choices=known_flake_lists,
    )

    parser.add_argument(
        "--add",
        type=str,
        nargs="+",
        help="Add tests to the flaky tests list",
        default=[],
    )

    parser.add_argument(
        "--remove",
        type=str,
        nargs="+",
        help="Remove tests from the flaky tests list",
        default=[],
    )

    args = parser.parse_args()
    main(args)
