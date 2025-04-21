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

import argparse
import os
import gitlab
import json
import requests
from urllib.parse import urlparse
import pyvips
import s3_upload_files

badges_data= '''
{
   "badges":[
      {
         "platform":"linux",
         "job_name":"build_and_test_desktop_release"
      },
      {
         "platform":"android",
         "job_name":"build_and_test_x64_debug"
      },
      {
         "platform":"windows",
         "job_name":"build_windows_release"
      },
      {
         "platform":"macos",
         "job_name":"mac_os_release_build_and_test"
      }
   ]
}
'''
jobs_badge_info = json.loads(badges_data)

def get_latest_pipeline_id(project, branch_name: str):
    """ Gets latest pipeline information from a branch
    """
    pipelines = project.pipelines.list(ref=branch_name, order_by="updated_at", scope="finished", get_all=True)
    if not pipelines:
        return None
    return pipelines[0].id

def get_pipeline_status(project, pipeline_id):
    pipeline = project.pipelines.get(pipeline_id)
    return pipeline.status

def generate_status_badge_image(name:str, status: str):
    if status == "success":
        color = "brightgreen"
    elif status == "failed":
        color = "red"
    elif status == "canceled":
        color = "white"
    else:
        color = "yellow"
    return f"https://img.shields.io/badge/{name}-{status}-{color}"

def update_pipeline_badge(project, badge_name:str, image_url:str):
    badges = project.badges.list()
    for item in badges:
        if item.name.lower() == badge_name:
            badge_id = item.id
            badge = project.badges.get(badge_id)
            badge.image_url = image_url
            badge.save()
    return None

def get_latest_release(project):
    release = project.releases.list()
    if not release:
        return None
    tag = release[0].tag_name
    major_version = tag.split('eyeo-release-')[1].split('.')[0]
    return major_version

def get_dev_version(project):
    dev_version = project.default_branch.split('-')[1]
    return dev_version

def generate_badge_image(image_url, badge_name):
    response = requests.get(image_url).content.decode("utf-8")
    file_name = f'{badge_name}.png'
    image = pyvips.Image.svgload_buffer(bytes(response,'UTF-8'))
    image.write_to_file(f"{badge_name}.png")
    return file_name


def upload_files_to_s3(file_name):
    bucket = os.environ.get('AWS_S3_BUCKET_CENTRAL_1')
    prefix = "wiki-resources/badges/"
    project_path = os.environ.get("CI_PROJECT_DIR")
    file_path = f"{project_path}/{file_name}"
    s3_upload_files.upload_files([file_path], bucket, prefix)

def main(args):

    gl = gitlab.Gitlab(
        args.gitlabhost,
        private_token=args.gitlab_private_token,
    )
    project = gl.projects.get(args.project_id)

    if args.update_release:
        release_version=get_latest_release(project)
        if release_version:
            badge_name="latest_release"
            image_url=f"https://img.shields.io/badge/{badge_name}-{release_version}-blue"
            update_pipeline_badge(project, badge_name, image_url)
            image_file_name = generate_badge_image(image_url, badge_name)
            upload_files_to_s3(image_file_name)

    if args.update_dev_version:
        dev_version = get_dev_version(project)
        badge_name="dev"
        image_url= f"https://img.shields.io/badge/{badge_name}-{dev_version}-blue"
        update_pipeline_badge(project, badge_name, image_url)
        image_file_name = generate_badge_image(image_url, badge_name)
        upload_files_to_s3(image_file_name)

    if args.branch_name:
        pipeline_id = get_latest_pipeline_id(project, args.branch_name)
        if pipeline_id:
            pipeline = project.pipelines.get(pipeline_id)
            for pipeline_job in pipeline.jobs.list():
                for job in jobs_badge_info['badges']:
                    if pipeline_job.name == job['job_name']:
                        image_url = generate_status_badge_image(job['platform'], pipeline_job.status)
                        update_pipeline_badge(project, job['platform'], image_url)
                        image_file_name = generate_badge_image(image_url, job['platform'])
                        upload_files_to_s3(image_file_name)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument(
        "--update-release",
        action='store_true',
        help="(int) Update release badge",
    )

    parser.add_argument(
        "--update-dev-version",
        action='store_true',
        help="(int) Update dev branch badge",
    )

    parser.add_argument(
        "--branch-name",
        type=str,
        help="(int) The branch name for getting pipeline information",
    )

    parser.add_argument(
        "--project-id",
        type=int,
        help="(int) The project ID to download from",
        default=os.environ.get("CI_PROJECT_ID"),
    )

    parser.add_argument(
        "--gitlabhost",
        type=str,
        help="The hostname of your gitlab server, including https://",
        default=os.environ.get("CI_SERVER_URL"),
    )

    parser.add_argument(
        "--gitlab-private-token",
        type=str,
        default=os.environ.get("CHROMIUM_GITLAB_COM_TOKEN"),
        help="Private token for authentication",
    )

    args = parser.parse_args()
    main(args)
