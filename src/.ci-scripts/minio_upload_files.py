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
Script to upload artifacts to Minio

"""

import os
import argparse
from minio import Minio
from urllib.parse import urlparse


def upload_file_to_minio(minio_client, bucket, local_file, minio_object):
    if not os.path.isfile(local_file):
        raise FileNotFoundError(f'{local_file} does not exist!')

    minio_client.fput_object(bucket, minio_object, local_file)


def main(args):
    access_key = os.environ.get("MINIO_ACCESS_KEY")
    secret_key = os.environ.get("MINIO_SECRET_KEY")
    minio_host = urlparse(os.environ.get("MINIO_HOST")).netloc
    minio_client = Minio(minio_host, access_key, secret_key)
    upload_file_to_minio(minio_client, args.minio_bucket, args.local_file,
                         args.minio_object)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument(
        "minio_bucket",
        type=str,
        help="The name of minio bucket",
        default="chromium-sdk",
    )

    parser.add_argument(
        "local_file",
        type=str,
        help="The local file to upload",
    )

    parser.add_argument(
        "minio_object",
        type=str,
        help="The object name in minio",
    )

    args = parser.parse_args()
    main(args)
