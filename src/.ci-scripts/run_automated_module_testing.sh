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

set -e  # Exit on error

MODULES_CONFIGURATIONS=(
    "base.patch,content_shell.patch:RUN_CONTENT_SHELL_RELEASE_BUILD"
    "base.patch,chrome_integration.patch,extension_api.patch:RUN_LINUX_INSTALLER_RELEASE_BUILD"
    "base.patch,chrome_integration.patch,android_api.patch,android_settings.patch:RUN_ANDROID_ARM64_APK_RELEASE_BUILD"
    "base.patch,webview_integration.patch:RUN_ANDROID_ARM64_WEBVIEW_RELEASE_BUILD"
)

setup_repository_for_module_testing() {
    setup_emulators
    local tag=$1
    if ! git ls-remote --tags origin | grep -q "$tag$"; then
        echo "Incorrect tag: ${tag}, does not exist on remote."
        exit 1
    fi
    git fetch origin "$tag" --depth 3
    git checkout "$tag"
    git reset HEAD~1
}

checkout_vanilla() {
    local tag=$1
    local eyeo_release_module_tag=$tag
    local eyeo_release_tag="${eyeo_release_module_tag%-modules}"

    if [[ $eyeo_release_tag == "$eyeo_release_module_tag" ]]; then
        echo "Incorrect tag: ${tag}, please provide module release tag"
        return 1
    fi

    local chromium_tag
    chromium_tag=$(echo "$eyeo_release_tag" | cut -d '-' -f3)
    local vanilla_branch="chromium-$chromium_tag-vanilla-automated"

    git fetch origin "$vanilla_branch" --depth 3
    git checkout "$vanilla_branch"
}

checkout_new_branch() {
    local branch_name=$1
    git branch -D "$branch_name" || true
    git checkout -b "$branch_name"
}

create_build_directories() {
    # Desktop
    gn gen --check --args="
        eyeo_intercept_debug_url=true
        is_cfi=false
        is_debug=false
        is_component_build=false
        use_remoteexec=true
        enable_nacl=false
        proprietary_codecs=true
        disable_fieldtrial_testing_config=true
        ffmpeg_branding=\"Chrome\"
        eyeo_application_name=\"app_name_from_ci_config\"
        eyeo_application_version=\"app_version_from_ci_config\"
        eyeo_telemetry_activeping_auth_token=\"${TELEMETRY_DEFAULT_SERVER_ACTIVEPING_AUTH_TOKEN}\"
        ${GN_EXTRA_ARGS} ${GN_SCHEDULE_DEFAULT_PIPELINE_ARGS}
    " out/Desktop-Release

    # Android
    gn gen --check --args="
        target_cpu=\"x64\"
        use_remoteexec=true
        enable_nacl=false
        dcheck_always_on=false
        target_os=\"android\"
        is_debug=false
        symbol_level=1
        proprietary_codecs=true
        ffmpeg_branding=\"Chrome\"
        eyeo_intercept_debug_url=true
        disable_fieldtrial_testing_config=true
        eyeo_telemetry_activeping_auth_token=\"${TELEMETRY_DEFAULT_SERVER_ACTIVEPING_AUTH_TOKEN}\"
        ${GN_EXTRA_ARGS} ${GN_SCHEDULE_DEFAULT_PIPELINE_ARGS}
    " out/Android-Release
}

apply_patch() {
    local patch_path=$1
    git am "eyeo_modules/$patch_path"
}

setup_emulators() {
    if [-z "$PORT}" ]; then
      export PORT=5555
    fi
    export DEVICES="localhost:${PORT}"
    .ci-scripts/setup_emulators.sh setup_emulator $PORT
}

build_and_run_tests() {
    # Desktop Build
    autoninja -j150 -C out/Desktop-Release chrome unit_tests components_unittests browser_tests components_browsertests

    # Desktop Tests
    python3 ${GIT_CLONE_PATH}/.ci-scripts/run_chromium_tests.py out/Desktop-Release unit_tests -- --gtest_filter="*Adblock*:*Eyeo*" --test-launcher-jobs=12
    python3 ${GIT_CLONE_PATH}/.ci-scripts/run_chromium_tests.py out/Desktop-Release components_unittests --  --gtest_filter="*Adblock*:*Eyeo*" --test-launcher-jobs=12
    python3 ${GIT_CLONE_PATH}/.ci-scripts/run_chromium_tests.py out/Desktop-Release browser_tests --  --gtest_filter="*Adblock*:*Eyeo*" --test-launcher-jobs=12 --no-sandbox | grep -v -e "error [0-9]\+.*Bad";

    # Android Build
    autoninja -j150 -C out/Android-Release chrome_public_apk unit_tests components_unittests chrome_public_test_apk components_perftests

    # Android Tests
    ./out/Android-Release/bin/run_unit_tests -d $DEVICES -v -f "*Abp*:*Adblock*" --gtest_output="xml:/data/user/0/org.chromium.native_test/unit_tests_report.xml" --app-data-file-dir out/Release/
    ./out/Android-Release/bin/run_components_unittests -d $DEVICES -v -f "*Abp*:*Adblock*" --gtest_output="xml:/data/user/0/org.chromium.native_test/components_unittests_report.xml" --app-data-file-dir out/Release/
    ./out/Android-Release/bin/run_chrome_public_test_apk -d $DEVICES --emulator-enable-network -v -A Feature=adblock --screenshot-directory /data/user/0/org.chromium.native_test/screenshots-chrome-apk
}

push_branch_and_trigger_test_clean_build() {
    local tag=$1
    local branch_prefix=$2
    local clean_build_target=$3
    local branch_name="${tag}${branch_prefix}"

    checkout_new_branch "$branch_name"
    git branch
    echo "git push origin \"$branch_name\" --force -o ci.variable=\"PIPELINE_TYPE=custom\" -o ci.variable=\"${clean_build_target}=true\" -o ci.variable=\"RUN_FOR_MODULE=true\""
    git push origin "$branch_name" --force -o ci.variable="PIPELINE_TYPE=custom" -o ci.variable="${clean_build_target}=true" -o ci.variable="RUN_FOR_MODULE=true"
}

cleanup_repo() {
    .ci-scripts/setup_emulators.sh teardown_emulator  $PORT
    git reset --hard
    rm -rf eyeo_modules
}

main() {
    local tag=$1
    setup_repository_for_module_testing "$tag"
    create_build_directories

    for module in "${MODULES_CONFIGURATIONS[@]}"; do
        IFS=":" read -r patches clean_build_target <<< "$module"

        checkout_vanilla "$tag"
        checkout_new_branch "module-testing-tmp-branch"

        branch_prefix=""
        IFS="," read -r -a patch_array <<< "$patches"
        for patch in "${patch_array[@]}"; do
            branch_prefix+="-${patch%%.*}"
            apply_patch "$patch"
            build_and_run_tests
        done

        push_branch_and_trigger_test_clean_build "$tag" "$branch_prefix" "$clean_build_target"
    done

    cleanup_repo
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    if [[ -z $1 ]]; then
        echo "Usage: $0 <tag>"
        exit 1
    fi
    main "$1"
fi
