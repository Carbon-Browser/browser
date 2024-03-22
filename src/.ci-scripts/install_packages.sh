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

# Install additional build dependencies.
# Detect if they have already been installed (deps rarely change)
for DEPS in install-build-deps.sh; do
if ! grep -q $(md5sum build/$DEPS) /installed_deps ; then
    md5sum build/$DEPS >> /installed_deps
    MISSING_DEPS="true"
fi
done
if [ "$MISSING_DEPS" ] ; then
    echo "WARNING: Missing dependencies detected, you may want to update the docker image used."
    ./build/install-build-deps.sh --android --no-prompt
fi
