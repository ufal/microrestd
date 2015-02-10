#!/bin/sh

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
