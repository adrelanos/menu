#! /bin/sh
#This very stupid shell script is Public Domain.

errorconf="returned an error code or did not exists at all. \
You need autoconf >= 2.50 and automake >= 1.4. Sorry!"

aclocal || { echo "aclocal $errorconf"; exit 1; }
autoconf || { echo "autoconf $errorconf"; exit 1; }
autoheader || { echo "autoheader $errorconf"; exit 1; }
automake -a || { echo "automake $errorconf"; exit 1; }
./configure --enable-maintainer-mode $*
