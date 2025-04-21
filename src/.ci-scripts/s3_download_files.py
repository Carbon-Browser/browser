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
Script to download files in S3

"""

import argparse
import os
import boto3


def download_with_retry(bucket,
                        prefix,
                        filename,
                        download_dir,
                        max_attempts=3):
    s3 = boto3.client('s3')
    s3_key = os.path.join(prefix, filename)

    if not os.path.isdir(download_dir):
        os.makedirs(download_dir, exist_ok=False)
    file_path = os.path.join(download_dir, filename)
    for attempt in range(max_attempts):
        try:
            s3.download_file(bucket, s3_key, file_path)
            print(
                f"Downloaded {filename} from {bucket}/{prefix} to {download_dir}"
            )
            break
        except Exception as e:
            if attempt < max_attempts - 1:
                print(
                    f"Failed to download {filename} from {bucket}, retrying ({attempt + 1}/{max_attempts})"
                )
            else:
                print(
                    f"Failed to download {filename} from {bucket} after {max_attempts} attempts"
                )
                raise e


def download_test_reports_from_bucket(bucket, prefix, download_dir=None):
    if not download_dir:
        download_dir = args.download_dir
    output_dir = os.path.join(download_dir, prefix)
    for item in [
            "unit_tests_failed.txt", "components_unittests_failed.txt",
            "browser_tests_failed.txt"
    ]:
        download_with_retry(bucket, prefix, item, output_dir)


def download_all_files_from_bucket(bucket, prefix, download_dir=None):
    s3 = boto3.client('s3')
    response = s3.list_objects_v2(Bucket=bucket, Prefix=prefix)
    if not download_dir:
        download_dir = args.download_dir
    output_dir = os.path.join(download_dir, prefix)
    for item in response.get('Contents', []):
        print(item['Key'])
        print(prefix)
        # Since the prefix is listed as object, we need to ignore it
        if item['Key'] != prefix:
            download_with_retry(bucket, prefix, os.path.basename(item['Key']),
                                output_dir)


def download_files_from_s3(bucket, prefix, filename, download_dir=None):
    if not download_dir:
        download_dir = args.download_dir

    download_with_retry(bucket, prefix, filename, download_dir)


def main(args):
    if args.download_all_files_from_bucket:
        download_all_files_from_bucket(args.bucket, args.prefix)
    else:
        download_files_from_s3(args.bucket, args.prefix, args.object_name)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument(
        "--download-all-files-from-bucket",
        action='store_true',
        help="(int) Download all files from a prefix inside a bucket",
    )

    parser.add_argument(
        "bucket",
        type=str,
        help="The name of S3 bucket",
        default=os.environ.get('AWS_S3_BUCKET_CENTRAL_1'),
    )

    parser.add_argument(
        "prefix",
        type=str,
        help="Prefix or path where the file is located in the bucket",
    )

    parser.add_argument(
        "--object_name",
        type=str,
        help="Name of the file to download",
    )

    parser.add_argument(
        "--download-dir",
        type=str,
        help="The directory to save the downloaded artifacts",
        default=os.environ.get("GIT_CLONE_PATH"),
    )

    args = parser.parse_args()
    main(args)
