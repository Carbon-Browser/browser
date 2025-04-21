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

# Setup emulator for testing
#!/bin/bash

CONTAINER_NAME=$CONTAINER_NAME
ANDROID_IMAGE=$ANDROID_IMAGE
BOOT_STATUS=""

create_emulator() {
    sudo modprobe binder_linux devices="binder,hwbinder,vndbinder"
    # Make sure same container doesn't exist and data directory is clean
    clean_up_container
    docker run -itd --rm --privileged --name $CONTAINER_NAME-$PORT --pull always -v ~/data-$PORT:/data -p $PORT:5555 $ANDROID_IMAGE
}

verify_device_is_ready() {
    elapsed=0
    timeout=60
    echo "Waiting for device on PORT ${PORT} to be fully booted..."
    while [[ $BOOT_STATUS != "1" ]] ; do
        BOOT_STATUS=$(adb -s localhost:${PORT} shell getprop sys.boot_completed 2>/dev/null)
        if [[ $elapsed -eq $timeout ]] ; then
            echo "Timeout of $timeout seconds reched. Device is not fully booted!"
            exit 1
        fi
        sleep 1
        elapsed=$((elapsed + 1)); \
    done
    echo "Device is fully booted and ready to be used."
}

prepare_emulator() {
    adb connect localhost:${PORT}
    if ! adb devices | grep -q "localhost:${PORT}"; then
        echo "ERROR: Device not connected!"
        exit 1
    fi
    verify_device_is_ready
    adb -s localhost:${PORT} root
    verify_device_is_ready
}

clean_up_container() {
    if [ "$(docker ps -q -f name=$CONTAINER_NAME-${PORT})" ]; then
        docker stop $CONTAINER_NAME-${PORT};
    fi
    sudo rm -rf ~/data-$PORT;
}

disconnect_emulator() {
    adb -s localhost:${PORT} disconnect
}

# Sometimes "ghost" devices are left behind, so we need to reboot them in order to clean up
disconnect_all_devices() {
    devices=$(adb devices | grep -w "device" | awk '{print $1}')
    for device in $devices; do
        adb -s $device -e reboot
    done
    adb kill-server
}

clean_up_env() {
    # Stop and remove all containers
    containers=$(docker ps -a -q --filter "name=^/emulator-")
    if [ -n "$containers" ]; then
        docker stop $containers
    fi
    # Remove all android data directories
    sudo rm -rf ~/data-*
}

# Parse command-line arguments
if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <command> <PORT>"
    echo "Commands: setup_emulator, teardown_emulator, clean_up_env"
    exit 1
fi

COMMAND=$1
PORT=$2

case "$COMMAND" in
    setup_emulator)
        if [[ -z "$PORT" ]]; then
            echo "PORT is required for setup_emulator"
            exit 1
        fi
        echo $PORT
        create_emulator
        prepare_emulator
        ;;
    teardown_emulator)
        if [[ -z "$PORT" ]]; then
            echo "PORT is required for teardown_emulator"
            exit 1
        fi
        clean_up_container
        disconnect_emulator
        ;;
    clean_up_env)
        clean_up_env
        ;;
    *)
        echo "Select a valid option: setup_emulator, teardown_emulator or clean_up_env"
        exit 1
        ;;
esac