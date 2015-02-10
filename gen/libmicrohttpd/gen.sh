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

#wget http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.39.tar.gz
#tar xf libmicrohttpd-0.9.39.tar.gz
#rm libmicrohttpd-0.9.39.tar.gz

SRC=libmicrohttpd-0.9.39/src
TGT=../../src/libmicrohttpd

cp MHD_config.h $TGT/
for f in include/autoinit_funcs.h include/microhttpd.h include/platform.h include/platform_interface.h include/w32functions.h platform/w32functions.cpp \
         microhttpd/connection.cpp microhttpd/connection.h microhttpd/daemon.cpp microhttpd/internal.cpp microhttpd/internal.h \
         microhttpd/memorypool.cpp microhttpd/memorypool.h microhttpd/postprocessor.cpp microhttpd/reason_phrase.cpp microhttpd/reason_phrase.h \
         microhttpd/response.cpp microhttpd/response.h microhttpd/tsearch.cpp microhttpd/tsearch.h; do
  cp $SRC/${f%pp} $TGT/${f##*/}
  cp $TGT/${f##*/} $TGT/${f##*/}.orig
done

echo All done.
#rm -rf libmicrohttpd-*/
