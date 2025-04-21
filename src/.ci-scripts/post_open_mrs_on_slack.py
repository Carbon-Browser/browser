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


class MergeRequests:

    def __init__(self):
        gl = gitlab.Gitlab(
            args.gitlabhost,
            private_token=args.gitlab_private_token,
        )
        project = gl.projects.get(args.project_id)
        mrs = project.mergerequests.list(state='opened')
        self.ready_reviewed = []
        self.ready_waiting = []
        self.draft_waiting = []
        self.do_not_merge_reviewed = []
        self.do_not_merge_waiting = []

        for mr in mrs:
            if "DO NOT MERGE" in mr.title:
                if "not_approved" in mr.detailed_merge_status:
                    self.do_not_merge_waiting.append(mr)
                else:
                    self.do_not_merge_reviewed.append(mr)
            elif mr.draft:
                if "not_approved" in mr.detailed_merge_status:
                    self.draft_waiting.append(mr)
                else:
                    self.ready_reviewed.append(mr)
            else:
                if "not_approved" in mr.detailed_merge_status:
                    self.ready_waiting.append(mr)
                else:
                    self.ready_reviewed.append(mr)


def getDadJokeOfADay():
    headers = {"Accept": "application/json"}
    response = requests.get("https://icanhazdadjoke.com/", headers=headers)

    if response.status_code == 200:
        joke = response.json()  # Parse the JSON response
        return "\nDaddy Joke Of a Day: \n" + joke['joke']
    else:
        return ""


def createMessage(merge_requests):

    def listMR(mrs):
        nonlocal message
        for mr in mrs:
            message += "    - " + mr.title + " <" + mr.web_url + "|link>\n"

    message = """
    Good Morning Dragons!
    Here is a list of open Merge Requests.
    """
    if merge_requests.ready_reviewed:
        message += "\n"
        message += str(len(merge_requests.ready_reviewed)) + (
            " MRs are " if len(merge_requests.ready_waiting) > 1 else
            " MR is ") + "ready to merge: \n"
        listMR(merge_requests.ready_reviewed)
    if merge_requests.ready_waiting:
        message += "\n"
        message += str(len(merge_requests.ready_waiting)) + (
            " MRs are " if len(merge_requests.ready_waiting) > 1 else
            " MR is ") + "calling for review: \n"
        listMR(merge_requests.ready_waiting)
    if merge_requests.draft_waiting:
        message += "\n"
        message += "" + str(len(merge_requests.draft_waiting)) + (
            " drafts " if len(merge_requests.ready_waiting) > 1 else
            " draft ") + " may be worth to look into: \n"
        listMR(merge_requests.draft_waiting)
    if merge_requests.do_not_merge_waiting:
        message += "\n"
        message += str(len(merge_requests.do_not_merge_waiting)) + (
            " [DO NOT MERGE] are " if len(merge_requests.ready_waiting) > 1
            else " [DO NOT MERGE] is ") + "waiting for review: \n"
        listMR(merge_requests.do_not_merge_waiting)
    return message


def postMessageToSlack(message):
    json_data = {
        'text': message,
    }

    response = requests.post(os.environ.get("SLACK_WEBHOOK_URL"),
                             json=json_data)
    print(response)


def main(args):
    postMessageToSlack(createMessage(MergeRequests()) + getDadJokeOfADay())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

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
