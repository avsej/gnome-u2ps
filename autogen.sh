#!/bin/sh

aclocal
glib-gettextize --force --copy
libtoolize -f -c
aclocal
intltoolize --force --copy
automake -a -c
autoconf

./configure $@
