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
Given a Chromium version, retrieves the test reports in a vanilla Chromium
branch. It first looks for an exact match, then for another release with the
same major version.
"""

import argparse
import os
import gitlab
import subprocess
import re
import requests
import minio_download_files
from urllib.parse import urlparse
from urllib3.exceptions import ReadTimeoutError


def get_pipeline_job(project, pipeline, job_name: str):
    """This function looks through all jobs in the given pipeline and returns
    the one with the given job_name (if its completed)
    """
    for pipeline_job in pipeline.jobs.list():
        # Look for the job and make sure it has finished (manual jobs may not
        # have started)
        if pipeline_job.name == job_name and pipeline_job.finished_at:
            return project.jobs.get(pipeline_job.id)
    return None

def get_gitlab_job(project, branch_name: str, job_name: str):
    """This takes a branch/tag name and returns a GitLab job object.

    TODO Add more information about what job is returned
    """
    # A generator that contains all successful pipelines
    pipelines = project.pipelines.list(all=True, ref=branch_name,
                                       status="success")
    # Look for the first pipeline that contains our job
    for pipeline in pipelines:
        job = get_pipeline_job(project, pipeline, job_name)
        if job:
            return job
    return None


def get_vanilla_test_gitlab_job(project, version: str, job_name: str):
    # Try exact version match
    exact_branch_name = "chromium-{}-vanilla-automated".format(version)
    print(f"Attempting retrieval for branch '{exact_branch_name}'...")
    job = get_gitlab_job(project, exact_branch_name, job_name)
    if job:
        return job

    # Try major version match
    major_version = version.split('.')[0]
    print("Could not find exact version match. "
          f"Attempting retrieval for major version {major_version}...")
    # The GitLab API search is limited to starting or trailing term, so the
    # response needs to be matched against the complete regex
    branch_pattern = "^chromium-{}.".format(major_version)
    returned_branches = project.branches.list(search=branch_pattern)
    candidate_branches = [
        b.name for b in returned_branches
        if re.match("chromium-\\d+.\\d+.\\d+.\\d+-vanilla-automated", b.name)
    ]

    # Check from newest to oldest version
    for approx_branch_name in reversed(candidate_branches):
        print(f"Attempting retrieval for branch '{approx_branch_name}'...")
        job = get_gitlab_job(project, approx_branch_name, job_name)
        if job:
            return job

    # No exact or approximate match found
    return None

def get_minio_test_files_folder(job):
    job_id=getattr(job, "id")
    pipeline_id=getattr(job, "pipeline")['id']
    project_path= os.environ.get("CI_PROJECT_PATH")
    return f"{project_path}/{pipeline_id}/{job_id}/out/Release/"

def download_artifacts_from_minio(bucket, minio_object, download_dir):
    minio_download_files.download_all_files_from_bucket(bucket, minio_object, download_dir)

def download_artifacts_from_gitlab(job, download_dir=None):
    """Downloads artifacts from the provided GitLab job into the download_dir
    using the GitLab API.
    """
    if not download_dir:
        download_dir = args.download_dir
    if not os.path.isdir(download_dir):
        os.makedirs(download_dir, exist_ok=False)

    zip_filename = "___artifacts.zip"
    print(f"Downloading artifacts from GitLab job: {job.web_url}")
    with open(zip_filename, "wb") as f:
        try:
            job.artifacts(streamed=True, action=f.write)
        except subprocess.TimeoutException:
            # Retry once
            print(f"Retrying downloading artifacts from GitLab job: "
                  f"{job.web_url}")
            job.artifacts(streamed=True, action=f.write)

    subprocess.run(["unzip", "-bd", download_dir, zip_filename])
    os.unlink(zip_filename)


def main(args):
    if (args.download_vanilla_reports_from_gitlab):
        download_timeout = 600  # Default timeout (in seconds) for APK downloads
        gl = gitlab.Gitlab(
            args.gitlab_host,
            private_token=args.gitlab_private_token,
            timeout=download_timeout
        )
        project = gl.projects.get(args.project_id)

        job = get_vanilla_test_gitlab_job(
            project, args.chromium_version, args.job_name
        )

        if not job:
            print(f"Could not find a matching job for {args.job_name}")
            exit(1)

        if not hasattr(job, "artifacts_file"):
            print(f"Could not find artifacts for job {args.job_name}")
            exit(2)

        download_artifacts_from_gitlab(job)
        print("Test reports successfully downloaded")
    else:
        bucket="chromium-sdk-archive"
        minio_prefix="chromium-"+args.chromium_version+"-vanilla-automated/out/Release/"
        minio_download_files.download_test_reports_from_bucket(bucket,minio_prefix,args.download_dir)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument(
        "--download_vanilla_reports_from_gitlab",
        action='store_true',
        help="(int) Download vanilla test reports from gitlab",
    )

    parser.add_argument(
        "chromium_version",
        type=str,
        help="The full Chromium version, e.g. 96.0.4664.92"
    )

    parser.add_argument(
        "download_dir",
        type=str,
        help="The directory to save the downloaded test reports"
    )

    parser.add_argument(
        "--gitlab-host",
        type=str,
        help="The hostname of the GitLab server, including https://",
        default=os.environ.get("CI_SERVER_URL")
    )

    parser.add_argument(
        "--gitlab-private-token",
        type=str,
        help="Private token for authentication agains the GitLab server",
        default=os.environ.get("CHROMIUM_GITLAB_COM_TOKEN")
    )

    parser.add_argument(
        "--project-id",
        type=int,
        help="(int) The project ID to download from",
        default=os.environ.get("CI_PROJECT_ID"),
    )

    parser.add_argument(
        "--job-name",
        type=str,
        help="The name of the job that produced the test reports",
        default="vanilla_build_and_test_desktop_release"
    )

    args = parser.parse_args()
    main(args)
