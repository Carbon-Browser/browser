#!/bin/bash

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

if [[ -z $1 ]]; then
  echo "Missing argument with snippets revision to checkout!"
  exit 1
fi

DEV_SNIPPETS_VERSION=$1

# Setup build node and npm deps for abp-snippets repo, requires 16+ version of node
curl -sL https://deb.nodesource.com/setup_20.x -o nodesource_setup.sh
chmod +x nodesource_setup.sh
sudo ./nodesource_setup.sh
sudo apt-get install -qy nodejs
# Check if npm is available before proceeding
if ! command -v npm &> /dev/null; then
  echo "npm could not be found, exiting."
  exit 1
fi
OLD_DIR=$PWD && cd components/adblock/core/resources/snippets
echo "Current snippets library files from DEPS:"
ls -al dist/*.jst
git config --global --add safe.directory /opt/ci/chromium-sdk/src/components/adblock/core/resources/snippets
git remote add dev_origin git@gitlab.com:eyeo/adblockplus/abp-snippets.git
git fetch dev_origin
git checkout ${DEV_SNIPPETS_VERSION}
npm install
npm run build
node ./bundle/dependencies.js
echo "New snippets library files from ${DEV_SNIPPETS_VERSION}:"
ls -al dist/*.jst
cd $OLD_DIR
