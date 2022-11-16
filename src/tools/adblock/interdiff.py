#!/usr/bin/env python3

# This file is part of eyeo Chromium SDK,
# Copyright (C) 2006-present eyeo GmbH
#
# eyeo Chromium SDK is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# eyeo Chromium SDK is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.

import subprocess, tempfile, argparse

class Interdiff:

    def __init__(self, args):
        self.source_from = args.source_from
        self.source_to = args.source_to
        self.dest_from = args.dest_from
        self.dest_to = args.dest_to
        self.git_root = args.root
        self.output = args.output

    def _execute(self, cmdline):
        res = subprocess.run(cmdline.split(), cwd=self.git_root,
            capture_output=True, check=False, text=True)
        return res

    def _to_temp_file(self, cmdline):
        f = tempfile.NamedTemporaryFile('w+t')
        res = self._execute(cmdline)
        f.write(res.stdout)
        f.flush()
        return f

    def _generate_diff(self, fn, is_source):
        res = self._to_temp_file('git diff {0} {1} -- {2}'
            .format(self.source_from if is_source else self.dest_from,
                self.source_to if is_source else self.dest_to, fn))
        return res

    def run(self):
        source_files = self._execute('git diff --name-only {0} {1}'
            .format(self.source_from, self.source_to)).stdout.split()
        dest_files = self._execute('git diff --name-only {0} {1}'
            .format(self.dest_from, self.dest_to)).stdout.split()
        all_files = sorted(set(source_files) | set(dest_files))

        if not all_files:
            print('Nothing to compare')
            return

        with open(self.output, 'w+t') as output:
            for fn in all_files:
                with self._generate_diff(fn, True) as source_temp, \
                        self._generate_diff(fn, False) as dest_temp:
                        res = self._execute('interdiff -q {0} {1}'
                            .format(source_temp.name, dest_temp.name))
                        if res.returncode != 0:
                            print('interdiff error, skipping: {0}'.format(fn))
                        else:
                            output.write(res.stdout)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Generates the difference between two git commit ranges.'
        'For migrations between 2 eyeo Chromium versions, each range '
        'corresponds to the difference between vanilla Chromium and the '
        'corresponding eyeo Chromium integration.')

    parser.add_argument('source_from', help='source range start')
    parser.add_argument('source_to', help='source range end')
    parser.add_argument('dest_from', help='destination range start')
    parser.add_argument('dest_to', help='destination range end')
    parser.add_argument('--output', help='output file name',
        default='interdiff.patch', metavar='filename')
    parser.add_argument('--root', help='git repository root',
        default=None, metavar='path')

    main = Interdiff(parser.parse_args())
    main.run()
