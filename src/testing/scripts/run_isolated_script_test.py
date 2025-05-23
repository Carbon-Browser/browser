#!/usr/bin/env vpython3
# Copyright 2015 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Runs a script that can run as an isolate (or not).

If optional argument --isolated-script-test-output=[FILENAME] is passed
to the script, json is written to that file in the format detailed in
//docs/testing/json-test-results-format.md.

If optional argument --isolated-script-test-filter=[TEST_NAMES] is passed to
the script, it should be a  double-colon-separated ("::") list of test names,
to run just that subset of tests.

This script is intended to be the base command invoked by the isolate,
followed by a subsequent Python script. It could be generalized to
invoke an arbitrary executable.
"""

import argparse
import json
import os
import sys
import tempfile

import common

# Some harnesses understand the --isolated-script-test arguments
# directly and prefer that they be passed through.
KNOWN_ISOLATED_SCRIPT_TEST_RUNNERS = {'run_web_tests.py', 'run_webgpu_cts.py'}

# Known typ test runners this script wraps. They need a different argument name
# when selecting which tests to run.
# TODO(dpranke): Detect if the wrapped test suite uses typ better.
KNOWN_TYP_TEST_RUNNERS = {
    'metrics_python_tests.py',
    'monochrome_python_tests.py',
    'run_blinkpy_tests.py',
    'run_mac_signing_tests.py',
    'run_mini_installer_tests.py',
    'test_suite_all.py',  # //tools/grit:grit_python_unittests
}

KNOWN_TYP_VPYTHON3_TEST_RUNNERS = {
    'monochrome_python_tests.py',
    'run_polymer_tools_tests.py',
    'test_suite_all.py',  # //tools/grit:grit_python_unittests
}

# pylint: disable=super-with-arguments


class BareScriptTestAdapter(common.BaseIsolatedScriptArgsAdapter):

  def __init__(self):
    super().__init__()
    # Arguments that are ignored, but added here because it's easier to ignore
    # them than to update bot configs to not pass them.
    common.add_emulator_args(self._parser)
    self._parser.add_argument('--coverage-dir', type=str, help='Unused')
    self._parser.add_argument('--use-persistent-shell',
                              action='store_true',
                              help='Unused')


class IsolatedScriptTestAdapter(common.BaseIsolatedScriptArgsAdapter):

  # Overriding parent implementation.
  # pylint: disable=no-self-use
  def generate_sharding_args(self, total_shards, shard_index):
    # This script only uses environment variable for sharding.
    del total_shards, shard_index  # unused
    return []
  # pylint: enable=no-self-use

  # Overriding parent implementation.
  # pylint: disable=no-self-use
  def generate_test_also_run_disabled_tests_args(self):
    return ['--isolated-script-test-also-run-disabled-tests']
  # pylint: enable=no-self-use

  # Overriding parent implementation.
  # pylint: disable=no-self-use
  def generate_test_filter_args(self, test_filter_str):
    return ['--isolated-script-test-filter=%s' % test_filter_str]
  # pylint: enable=no-self-use

  # Overriding parent implementation.
  # pylint: disable=no-self-use
  def generate_test_output_args(self, output):
    return ['--isolated-script-test-output=%s' % output]
  # pylint: enable=no-self-use

  # Overriding parent implementation.
  # pylint: disable=no-self-use
  def generate_test_launcher_retry_limit_args(self, retry_limit):
    return ['--isolated-script-test-launcher-retry-limit=%d' % retry_limit]
  # pylint: enable=no-self-use

  # Overriding parent implementation.
  # pylint: disable=no-self-use
  def generate_test_repeat_args(self, repeat_count):
    return ['--isolated-script-test-repeat=%d' % repeat_count]
  # pylint: enable=no-self-use


class TypUnittestAdapter(common.BaseIsolatedScriptArgsAdapter):

  def __init__(self):
    super(TypUnittestAdapter, self).__init__()
    self._temp_filter_file = None

  # Overriding parent implementation.
  # pylint: disable=no-self-use
  def generate_sharding_args(self, total_shards, shard_index):
    # This script only uses environment variable for sharding.
    del total_shards, shard_index  # unused
    return []
  # pylint: enable=no-self-use

  def generate_test_filter_args(self, test_filter_str):
    filter_list = common.extract_filter_list(test_filter_str)
    self._temp_filter_file = tempfile.NamedTemporaryFile(mode='w', delete=False)
    self._temp_filter_file.write('\n'.join(filter_list))
    self._temp_filter_file.close()
    arg_name = 'test-list'
    if any(r in self.rest_args[0] for r in KNOWN_TYP_TEST_RUNNERS):
      arg_name = 'file-list'

    return ['--%s=' % arg_name + self._temp_filter_file.name]

  # Overriding parent implementation.
  # pylint: disable=no-self-use
  def generate_test_output_args(self, output):
    return ['--write-full-results-to', output]
  # pylint: enable=no-self-use

  # Overriding parent implementation.
  # pylint: disable=no-self-use
  def generate_test_launcher_retry_limit_args(self, retry_limit):
    return ['--isolated-script-test-launcher-retry-limit=%d' % retry_limit]
  # pylint: enable=no-self-use

  # Overriding parent implementation.
  # pylint: disable=no-self-use
  def generate_test_repeat_args(self, repeat_count):
    return ['--isolated-script-test-repeat=%d' % repeat_count]
  # pylint: enable=no-self-use

  def clean_up_after_test_run(self):
    if self._temp_filter_file:
      os.unlink(self._temp_filter_file.name)

  def select_python_executable(self):
    if any(r in self.rest_args[0] for r in KNOWN_TYP_VPYTHON3_TEST_RUNNERS):
      return 'vpython3.bat' if sys.platform == 'win32' else 'vpython3'
    return super(TypUnittestAdapter, self).select_python_executable()


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--script-type', choices=['isolated', 'typ', 'bare'])
  args, _ = parser.parse_known_args()

  kind = args.script_type
  if not kind:
    if any(r in sys.argv[1] for r in KNOWN_ISOLATED_SCRIPT_TEST_RUNNERS):
      kind = 'isolated'
    else:
      kind = 'typ'

  if kind == 'isolated':
    adapter = IsolatedScriptTestAdapter()
  elif kind == 'typ':
    adapter = TypUnittestAdapter()
  else:
    adapter = BareScriptTestAdapter()
  return adapter.run_test()


# This is not really a "script test" so does not need to manually add
# any additional compile targets.
def main_compile_targets(args):
  json.dump([], args.output)


if __name__ == '__main__':
  # Conform minimally to the protocol defined by ScriptTest.
  if 'compile_targets' in sys.argv:
    funcs = {
        'run': None,
        'compile_targets': main_compile_targets,
    }
    sys.exit(common.run_script(sys.argv[1:], funcs))
  sys.exit(main())
