#!/bin/bash
# ls -1 /usr/include/*.h > /tmp/ctags-files
# find /usr/include/x86_64-linux-gnu | grep h$ >> /tmp/ctags-files
find `pwd` | grep -e "[.]c$" > .ctags-files
find `pwd` | grep -e "[.]h$" >> .ctags-files
ctags -L .ctags-files

# ctags *.h *.c /usr/include/*.h /usr/include/x86_64-linux-gnu/sys/*.h
