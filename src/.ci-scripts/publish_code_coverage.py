#!/usr/bin/env python3

"""
This script publishes the code coverage results into a Mattermost channel.
"""

import argparse
import os
import glob
import csv
import statistics
import mattermost
import gitlab
import subprocess
import re
from pathlib import Path


def generate_results_table(metrics_file):
    """
    This takes the coverage-related entries from the metrics file
    and returns a multiline string suitable for posting to Mattermost
    """
    message = []
    message.append("| Code coverage metric | Percentage |")
    message.append("| :------------ | ------------:|")

    with open(metrics_file) as infile:
        data = infile.readlines()
        for line in data:
            fields = line.split()
            # Make the metric name more human readable
            metric = fields[0].replace('_', ' ').title()
            measurement = fields[1]
            message.append("| " + metric + " | " + measurement + " |")

    return "\n".join(message)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "--metricsfile",
        type=str,
        help="Path to file containing coverage metrics",
        default=os.environ.get("METRICS_FILE"),
    )
    parser.add_argument(
        "--mattermosttoken",
        type=str,
        help="Mattermost webhook token",
        default=os.environ.get("MATTERMOST_TOKEN"),
    )
    parser.add_argument(
        "--mattermostchannelid",
        type=str,
        help="Mattermost channel ID (not channel name!)",
        default=os.environ.get("MATTERMOST_CHANNEL_ID"),
    )

    args = parser.parse_args()

    gl = gitlab.Gitlab(
        os.environ["CI_SERVER_URL"], private_token=os.environ["CHROMIUM_GITLAB_TOKEN"],
    )
    gl.auth()

    code_coverage_table = generate_results_table(args.metricsfile)
    print(code_coverage_table)

    # Connect to Mattermost
    if args.mattermosttoken and args.mattermostchannelid:
        mm = mattermost.MMApi("https://mattermost.eyeo.com/api")
        mm.login(bearer=args.mattermosttoken)
        mm.create_post(args.mattermostchannelid, code_coverage_table)
