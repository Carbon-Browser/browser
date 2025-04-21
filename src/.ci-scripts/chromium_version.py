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

import os


def get_chromium_version():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    file_path = os.path.join(script_dir, "../chrome/VERSION")
    with open(file_path, 'r') as file:
        version = {}
        for line in file:
            key, value = line.strip().split('=')
            version[key] = value
    return f"{version['MAJOR']}.{version['MINOR']}.{version['BUILD']}.{version['PATCH']}"


if __name__ == "__main__":
    print(get_chromium_version())
