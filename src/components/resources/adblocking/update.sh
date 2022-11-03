#!/bin/bash
wget https://easylist-downloads.adblockplus.org/easylist-minified.txt
sed -i 's/! Expires: .*//' easylist-minified.txt
mv easylist-minified.txt easylist.txt

wget https://easylist-downloads.adblockplus.org/exceptionrules-minimal.txt
sed -i 's/! Expires: .*//' exceptionrules-minimal.txt
mv exceptionrules-minimal.txt exceptionrules.txt

