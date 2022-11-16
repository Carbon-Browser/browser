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

from datetime import timedelta
from minio import Minio
from urllib.parse import urlparse

def get_presigned_artifact_url(bucket, minio_object):
    access_key = os.environ.get("MINIO_ACCESS_KEY")
    secret_key = os.environ.get("MINIO_SECRET_KEY")
    minio_host = urlparse(os.environ.get("MINIO_HOST")).netloc
    minio_client = Minio(minio_host, access_key, secret_key)
    url = minio_client.get_presigned_url(
        "GET",
        bucket,
        minio_object,
        expires=timedelta(days=1),
    )
    print(url)

def main(args):

    get_presigned_artifact_url(args.minio_bucket, args.minio_object)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter
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

    args = parser.parse_args()
    main(args)
