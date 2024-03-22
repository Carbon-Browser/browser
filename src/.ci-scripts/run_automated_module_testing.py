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

from git import Repo
import shutil
import sys

if len(sys.argv) != 2 or sys.argv[1] == "-h" or sys.argv[1] == "--help":
  print ("Usage: TBD")
  sys.exit()

repo = Repo(".")
origin = repo.remote(name="origin")

if not any(remote_tag.endswith(sys.argv[1]) for remote_tag in repo.git.ls_remote("--tags", "origin").split()):
  print("Incorrect tag, does not exist on remote.")
  sys.exit()

try:
  EYEO_RELEASE_MODULE_TAG = sys.argv[1]
  EYEO_RELEASE_TAG = EYEO_RELEASE_MODULE_TAG.rstrip("-modules")
  assert not EYEO_RELEASE_TAG == EYEO_RELEASE_MODULE_TAG
  CHROMIUM_TAG = EYEO_RELEASE_TAG.split("-")[2]
except:
  print("Incorrect tag, please provide module release tag")


# Take advantage of how git handles unstage new directory, it does not conflict
# on branch switching. It simplifies applying modules.
repo.git.checkout(EYEO_RELEASE_MODULE_TAG)
repo.git.reset("HEAD~1")
VANILLA_BRANCH = "chromium-" + CHROMIUM_TAG + "-vanilla-automated"

# Remove local vanilla branch if exist
try:
  repo.git.branch("-D", VANILLA_BRANCH)
except:
  pass

try:
  # Fetch vanilla-automated branch
  origin.fetch(VANILLA_BRANCH)

  # Create branches for each module combination on top of the vanilla branch,
  # If branches are present locally, remove them and proceed.
  modules_branches_to_create = [
      EYEO_RELEASE_TAG + "-base-module-branch",
      EYEO_RELEASE_TAG + "-base-chrome-module-branch",
      EYEO_RELEASE_TAG + "-base-chrome-android-api-module-branch",
      EYEO_RELEASE_TAG + "-base-chrome-android-settings-module-branch",
      EYEO_RELEASE_TAG + "-base-chrome-desktop-module-branch",
      EYEO_RELEASE_TAG + "-base-webview-module-branch"
  ]

  for branch in modules_branches_to_create:
      if any(local_branch.name.endswith(branch) for local_branch in repo.branches):
          repo.delete_head(branch, force=True)

  # Vanilla + base
  repo.git.checkout(VANILLA_BRANCH)
  repo.git.checkout("-b", EYEO_RELEASE_TAG + "-base-module-branch")
  repo.git.am("eyeo_modules/base.patch")
  origin.push(EYEO_RELEASE_TAG + "-base-module-branch", force=True,
    o=[
      "ci.variable=PIPELINE_TYPE=custom",
      "ci.variable=RUN_FOR_MODULE=true",
      "ci.variable=RUN_FOR_WEBVIEW_MODULE=false",
      "ci.variable=RUN_ANDROID_X64_DEBUG_JOB=true",
      "ci.variable=RUN_DESKTOP_RELEASE_TEST_JOB=true",
      "ci.variable=RUN_APPIUM_TESTS=false",
      "ci.variable=RUN_SELENIUM_TESTS=false",
    ])

  # Vanilla + base + chrome
  repo.git.checkout(VANILLA_BRANCH)
  repo.git.checkout("-b", EYEO_RELEASE_TAG + "-base-chrome-module-branch")
  repo.git.am("eyeo_modules/base.patch")
  repo.git.am("eyeo_modules/chrome_integration.patch")
  origin.push(EYEO_RELEASE_TAG + "-base-chrome-module-branch", force=True,
    o=[
      "ci.variable=PIPELINE_TYPE=custom",
      "ci.variable=RUN_FOR_MODULE=true",
      "ci.variable=RUN_FOR_WEBVIEW_MODULE=false",
      "ci.variable=RUN_ANDROID_X64_DEBUG_JOB=true",
      "ci.variable=RUN_DESKTOP_RELEASE_TEST_JOB=true",
      "ci.variable=RUN_APPIUM_TESTS=false",
      "ci.variable=RUN_SELENIUM_TESTS=false",
    ])

  # Vanilla + base + chrome + android API
  repo.git.checkout(VANILLA_BRANCH)
  repo.git.checkout("-b", EYEO_RELEASE_TAG + "-base-chrome-android-api-module-branch")
  repo.git.am("eyeo_modules/base.patch")
  repo.git.am("eyeo_modules/chrome_integration.patch")
  repo.git.am("eyeo_modules/android_api.patch")
  origin.push(EYEO_RELEASE_TAG + "-base-chrome-android-api-module-branch", force=True,
    o=[
      "ci.variable=PIPELINE_TYPE=custom",
      "ci.variable=RUN_FOR_MODULE=true",
      "ci.variable=RUN_FOR_WEBVIEW_MODULE=false",
      "ci.variable=RUN_ANDROID_X64_DEBUG_JOB=true",
      "ci.variable=RUN_DESKTOP_RELEASE_TEST_JOB=true",
      "ci.variable=RUN_APPIUM_TESTS=true",
      "ci.variable=RUN_SELENIUM_TESTS=false",
    ])

  # Vanilla + base + chrome + android API + android UI
  repo.git.checkout(VANILLA_BRANCH)
  repo.git.checkout("-b", EYEO_RELEASE_TAG + "-base-chrome-android-settings-module-branch")
  repo.git.am("eyeo_modules/base.patch")
  repo.git.am("eyeo_modules/chrome_integration.patch")
  repo.git.am("eyeo_modules/android_api.patch")
  repo.git.am("eyeo_modules/android_settings.patch")
  origin.push(EYEO_RELEASE_TAG + "-base-chrome-android-settings-module-branch", force=True,
    o=[
      "ci.variable=PIPELINE_TYPE=custom",
      "ci.variable=RUN_FOR_MODULE=true",
      "ci.variable=RUN_FOR_WEBVIEW_MODULE=false",
      "ci.variable=RUN_ANDROID_X64_DEBUG_JOB=true",
      "ci.variable=RUN_DESKTOP_RELEASE_TEST_JOB=true",
      "ci.variable=RUN_APPIUM_TESTS=true",
      "ci.variable=RUN_SELENIUM_TESTS=false",
    ])

  # Vanilla + base + chrome + desktop extension API
  repo.git.checkout(VANILLA_BRANCH)
  repo.git.checkout("-b", EYEO_RELEASE_TAG + "-base-chrome-desktop-module-branch")
  repo.git.am("eyeo_modules/base.patch")
  repo.git.am("eyeo_modules/chrome_integration.patch")
  repo.git.am("eyeo_modules/extension_api.patch")
  origin.push(EYEO_RELEASE_TAG + "-base-chrome-desktop-module-branch", force=True,
    o=[
      "ci.variable=PIPELINE_TYPE=custom",
      "ci.variable=RUN_FOR_MODULE=true",
      "ci.variable=RUN_FOR_WEBVIEW_MODULE=false",
      "ci.variable=RUN_ANDROID_X64_DEBUG_JOB=true",
      "ci.variable=RUN_DESKTOP_RELEASE_TEST_JOB=true",
      "ci.variable=RUN_APPIUM_TESTS=false",
      "ci.variable=RUN_SELENIUM_TESTS=true",
    ])


  # Vanilla + base + webview
  repo.git.checkout(VANILLA_BRANCH)
  repo.git.checkout("-b", EYEO_RELEASE_TAG + "-base-webview-module-branch")
  repo.git.am("eyeo_modules/base.patch")
  repo.git.am("eyeo_modules/webview_integration.patch")
  origin.push(EYEO_RELEASE_TAG + "-base-webview-module-branch", force=True,
    o=[
      "ci.variable=PIPELINE_TYPE=custom",
      "ci.variable=RUN_FOR_MODULE=true",
      "ci.variable=RUN_FOR_WEBVIEW_MODULE=true",
      "ci.variable=RUN_ANDROID_X64_DEBUG_JOB=true",
      "ci.variable=RUN_DESKTOP_RELEASE_TEST_JOB=true",
      "ci.variable=RUN_APPIUM_TESTS=false",
      "ci.variable=RUN_SELENIUM_TESTS=false",
    ])

except:
  print("Generating automated module testing pipelines failed!")
  shutil.rmtree('eyeo_modules', ignore_errors=True)
  sys.exit()

# Clean up
shutil.rmtree('eyeo_modules', ignore_errors=True)
