#!/usr/bin/env python3
# Copyright 2022 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
import sys
import os
import PRESUBMIT

sys.path.append(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..'))
from PRESUBMIT_test_mocks import MockInputApi, MockOutputApi, MockAffectedFile


class PresubmitTest(unittest.TestCase):
    def testCheckForDoctypeHTML(self):
        """This verifies that we correctly identify missing DOCTYPE html tags.
        """

        file1 = MockAffectedFile("some/dir/file1.html", [
            "<!DOCTYPE html>", "<html>", "<body>", "<p>Test</p>", "</body>",
            "</html>"
        ])
        file2 = MockAffectedFile(
            "some/dir2/file2.html",
            ["<html>", "<body>", "<p>Test</p>", "</body>", "</html>"])
        file3 = MockAffectedFile("file3.html", [
            "<!--Some comment-->", "<!docTYPE    htML>", "<html>", "<body>",
            "<p>Test</p>", "</body>", "</html>"
        ])
        file4 = MockAffectedFile("dir/file4.html",
                                 ["<script></script>", "<!DOCTYPE html>"])
        file5 = MockAffectedFile("file5.html", [])
        file6 = MockAffectedFile(
            "file6.not_html",
            ["<html>", "<body>", "<p>Test</p>", "</body>", "</html>"])
        file7 = MockAffectedFile("file7.html", [
            "<!DOCTYPE html   >", "<html>", "<body>", "<p>Test</p>", "</body>",
            "</html>"
        ])
        file8 = MockAffectedFile("file8.html", [
            "<!DOCTYPE html FOOBAR>", "<html>", "<body>", "<p>Test</p>",
            "</body>", "</html>"
        ])
        file9 = MockAffectedFile(
            "some/dir/quirk-file9.html",
            ["<html>", "<body>", "<p>Test</p>", "</body>", "</html>"])
        file10 = MockAffectedFile(
            "old/file10.html",
            ["<html>", "<body>", "<p>New content</p>", "</body>", "</html>"],
            ["<html>", "<body>", "<p>Old content</p>", "</body>", "</html>"],
            action="M")

        mock_input_api = MockInputApi()
        mock_input_api.files = [
            file1, file2, file3, file4, file5, file6, file7, file8, file9,
            file10
        ]
        messages = PRESUBMIT._CheckForDoctypeHTML(mock_input_api,
                                                  MockOutputApi())

        self.assertEqual(4, len(messages))
        for i, file in enumerate([file2, file4, file5, file8]):
            self.assertEqual("error", messages[i].type)
            self.assertIn("\"%s\"" % file.LocalPath(), messages[i].message)


if __name__ == "__main__":
    unittest.main()
