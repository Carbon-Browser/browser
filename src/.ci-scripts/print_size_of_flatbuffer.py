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

import argparse
import gzip
import os
import shutil
import subprocess
import tempfile


def main():
    # Parse arguments
    parser = argparse.ArgumentParser(
        description='Run converter executable on a gzip file.')
    parser.add_argument('converter_executable',
                        type=str,
                        help='Location of the converter executable')
    parser.add_argument('input_file',
                        type=str,
                        help='Location of the input gzip file')
    args = parser.parse_args()

    # Create a temporary folder
    temp_folder = tempfile.mkdtemp()

    try:
        # Extract the input file (gzip) into a text file in the temp folder
        extracted_file_path = os.path.join(temp_folder, 'extracted_file.txt')
        with gzip.open(args.input_file, 'rb') as f_in:
            with open(extracted_file_path, 'wb') as f_out:
                shutil.copyfileobj(f_in, f_out)

        # Run the converter executable
        output_file_path = os.path.join(temp_folder, 'extracted_file.fb')
        subprocess.run([
            args.converter_executable, extracted_file_path,
            'https://easylist-downloads.adblockplus.org/extracted_file.txt',
            output_file_path
        ],
                       check=True)

        # Write the size of the output file to stdout
        output_file_size = os.path.getsize(output_file_path)
        print(f'{output_file_size}')

    finally:
        # Destroy the temporary folder
        shutil.rmtree(temp_folder)


if __name__ == '__main__':
    main()
