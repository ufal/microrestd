#!/bin/sh

# This file is part of MicroRestD <http://github.com/ufal/microrestd/>.
#
# Copyright 2015 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University in Prague, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set -e

wget https://github.com/zeux/pugixml/releases/download/v1.5/pugixml-1.5.tar.gz
tar xf pugixml-1.5.tar.gz
rm pugixml-1.5.tar.gz

SRC=pugixml-1.5/src
TGT=../../src/pugixml

for f in pugiconfig.h pugixml.h pugixml.cpp; do
  # Copy the file to TGT with given header
  cat >$TGT/$f <<EOF
// Modified by Milan Straka <straka@ufal.mff.cuni.cz>
// Changes: - added ufal::microrestd namespace
//          - matching changed to ignore XML namespaces
//          - XPATH and STL module completely removed

EOF
  cat $SRC/$f* >>$TGT/$f

  # Remove the XPATH module
  perl -pe '
    if (/#if.*NO_(XPATH|STL)/) {
      print;
      print "// Removed the XPATH and STL module, by Milan Straka\n";
      my $stack = 1;
      while (<>) {
        $stack++ if /#if/;
        $stack-- if /#endif/;
        last unless $stack;
      }
    }
  ' -i $TGT/$f

  # Apply patch
  patch $TGT/$f patches/$f.patch
done

echo All done.
rm -rf pugixml-1.5/
