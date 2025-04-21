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
Script to download artifacts from Minio

"""

import argparse
import os

from minio import Minio
from urllib.parse import urlparse
from urllib3.exceptions import ReadTimeoutError


def download_with_retry(minio_client, bucket, object_name, file_path):
    try:
        minio_client.fget_object(bucket_name=bucket,
                                 object_name=object_name,
                                 file_path=file_path)
    except ReadTimeoutError:
        # Retry once
        print(f"Retrying downloading {object_name} from minio")
        minio_client.fget_object(bucket_name=bucket,
                                 object_name=object_name,
                                 file_path=file_path)


def download_test_reports_from_bucket(minio_client,
                                      bucket,
                                      minio_dir,
                                      download_dir=None):
    if not download_dir:
        download_dir = args.download_dir
    if not os.path.isdir(download_dir):
        os.makedirs(download_dir, exist_ok=False)
    for item in [
            "unit_tests_failed.txt", "components_unittests_failed.txt",
            "browser_tests_failed.txt"
    ]:
        download_with_retry(minio_client, bucket, minio_dir + item,
                            f"{download_dir}/{minio_dir + item}")


def download_all_files_from_bucket(minio_client,
                                   bucket,
                                   minio_object,
                                   download_dir=None):
    if not download_dir:
        download_dir = args.download_dir
    if not os.path.isdir(download_dir):
        os.makedirs(download_dir, exist_ok=False)
    for item in minio_client.list_objects(bucket, prefix=minio_object):
        download_with_retry(minio_client, bucket, item.object_name,
                            f"{download_dir}/{item.object_name}")


def download_files_from_minio(minio_client,
                              bucket,
                              minio_object,
                              download_dir=None):
    if not download_dir:
        download_dir = args.download_dir
    if not os.path.isdir(download_dir):
        os.makedirs(download_dir, exist_ok=False)

    output_file = f"{download_dir}/{minio_object}"
    if os.path.exists(output_file):
        raise FileExistsError(f'{output_file} already exists!')

    download_with_retry(minio_client, bucket, minio_object, output_file)

def main(args):
    access_key = os.environ.get("MINIO_ACCESS_KEY")
    secret_key = os.environ.get("MINIO_SECRET_KEY")
    minio_host = urlparse(os.environ.get("MINIO_HOST")).netloc
    minio_client = Minio(minio_host, access_key, secret_key)
    if args.download_all_files_from_bucket:
        download_all_files_from_bucket(minio_client, args.minio_bucket,
                                       args.minio_object)
    else:
        download_files_from_minio(minio_client, args.minio_bucket,
                                  args.minio_object)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument(
        "--download-all-files-from-bucket",
        action='store_true',
        help="(int) Download all files from a bucket",
    )

    parser.add_argument(
        "minio_bucket",
        type=str,
        help="The name of minio bucket",
        default="chromium-sdk",
    )

    parser.add_argument(
        "minio_object",
        type=str,
        help="Path to minio object",
    )

    parser.add_argument(
        "--download-dir",
        type=str,
        help="The directory to save the downloaded artifacts",
        default=os.environ.get("GIT_CLONE_PATH"),
    )

    args = parser.parse_args()
    main(args)
