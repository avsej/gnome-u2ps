#!/bin/sh

if [ -f /usr/share/automake-1.8/mkinstalldirs ]; then
  ln -sf /usr/share/automake-1.8/mkinstalldirs
fi
aclocal
glib-gettextize --force --copy
libtoolize -f -c
aclocal
intltoolize --force --copy
automake -a -c
autoconf

./configure $@
