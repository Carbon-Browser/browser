#!/bin/bash
# Install additional build dependencies.
# Detect if they have already been installed (deps rarely change)
for DEPS in install-build-deps-android.sh install-build-deps.sh; do
if ! grep -q $(md5sum build/$DEPS) /installed_deps ; then
    md5sum build/$DEPS >> /installed_deps
    MISSING_DEPS="true"
fi
done
if [ "$MISSING_DEPS" ] ; then
    echo "WARNING: Missing dependencies detected, you may want to update the docker image used."
    chmod +x ./build/*.sh && ./build/install-build-deps-android.sh --no-prompt
fi