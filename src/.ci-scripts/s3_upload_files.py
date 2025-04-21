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
Script to upload artifacts to S3

"""

import argparse
import base64
import boto3
import os
import argparse
import glob
import s3_get_presigned_url

job_name = os.environ.get('CI_JOB_NAME_SLUG')
s3 = boto3.client('s3')


def upload_files(file_paths, bucket_name, prefix=None, file_names=None):
    # Make sure file_paths and file_names are lists before iterating
    if not isinstance(file_paths, list):
        file_paths = [file_paths]
    if file_names and not isinstance(file_names, list):
        file_names = [file_names]

    # Expand file paths in case there is a wildcard in the path
    expanded_file_paths = []
    for file_path in file_paths:
        if os.path.isdir(file_path):
            folder_basename = os.path.basename(file_path)
            folder_prefix = os.path.join(prefix, folder_basename)
            for root, _, files in os.walk(file_path):
                for file in files:
                    expanded_file_paths.append(
                        (os.path.join(root, file), folder_prefix))
        else:
            expanded_file_paths.extend([(path, prefix)
                                        for path in glob.glob(file_path)])

    for file_path, folder_prefix in expanded_file_paths:
        if os.path.isdir(file_path):
            s3_key = os.path.join(
                folder_prefix,
                os.path.relpath(file_path, start=os.path.dirname(file_path)))
        else:
            s3_key = os.path.join(prefix, os.path.basename(file_path))
        try:
            s3.upload_file(file_path, bucket_name, s3_key)
            # Generate a pre-signed URL only for specific file extensions
            print(f"{file_path} was uploaded")
            if file_path.endswith(
                ('.zip', '.apk', '.deb', '.dmg', '.exe', '.tar')):
                expiration_in_days = 1
                # Get AWS credentials from Secrets Manager so URLs can be valid for more than 1 hour
                credentials = s3_get_presigned_url.get_aws_credentials()
                aws_access_key_id = credentials['AWS_ACCESS_KEY_ID']
                aws_secret_access_key = credentials['AWS_SECRET_ACCESS_KEY']
                url = s3_get_presigned_url.get_presigned_artifact_url(
                    bucket_name, s3_key, expiration_in_days, aws_access_key_id,
                    aws_secret_access_key)
                # Due to Gitlab limitations, URL needs to be encoded so it can be accessed
                encoded_url = base64.b64encode(url.encode()).decode()
                print(f"Base64-encoded URL for {file_path}: {encoded_url}")
        except Exception as e:
            print(f"Error uploading {file_path}: {e}")
    print("Use a Base64 decoder to get the URL for the needed artifact.")
    print("All files were successfully uploaded.")


def set_lifecycle_policy(bucket_name, prefix, expiration_days):
    lifecycle_policy = {
        'Rules': [{
            'ID': 'Expire artifacts after a certain number of days',
            'Prefix': prefix,
            'Status': 'Enabled',
            'Expiration': {
                'Days': expiration_days
            }
        }]
    }

    s3.put_bucket_lifecycle_configuration(
        Bucket=bucket_name, LifecycleConfiguration=lifecycle_policy)

    print(
        f"Set lifecycle policy on {bucket_name}/{prefix} to expire after {expiration_days} days"
    )


def main(args):
    if args.archive_artifacts:
        ci_commit_tag = os.environ.get('CI_COMMIT_TAG')
        if ci_commit_tag:
            prefix = f"builds-archive/{ci_commit_tag}/{job_name}"
        else:
            prefix = f"builds-archive/{os.environ.get('CI_COMMIT_BRANCH')}/{job_name}"
    else:
        prefix = args.prefix

    upload_files(args.file_paths, args.bucket_name, prefix)

    # Set lifecycle policy only for temporary artifacts
    if not args.archive_artifacts or not args.upload_badges:
        set_lifecycle_policy(args.bucket_name, prefix, args.expiration_days)


if __name__ == "__main__":
    prefix = f"builds/{os.environ.get('CI_PIPELINE_ID')}/{os.environ.get('CI_JOB_ID')}/{job_name}"
    parser = argparse.ArgumentParser(
        description="Upload files to an S3 bucket")
    parser.add_argument("bucket_name",
                        help="Name of the S3 bucket",
                        type=str,
                        default=os.environ.get('AWS_S3_BUCKET_CENTRAL_1'))
    parser.add_argument("file_paths",
                        nargs='+',
                        help="List of file paths to upload")
    parser.add_argument("--prefix",
                        help="Prefix to add to the S3 key",
                        default=prefix)
    parser.add_argument("--expiration_days",
                        type=int,
                        help="Number of days to keep the files",
                        default=1)
    parser.add_argument("--archive_artifacts",
                        action='store_true',
                        help="Archive artifacts to S3")
    parser.add_argument("--upload_badges",
                        action='store_true',
                        help="Upload badges to S3")

    args = parser.parse_args()  # Parse the command-line arguments
    main(args)
