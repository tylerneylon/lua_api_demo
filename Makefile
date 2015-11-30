# Makefile
#
# This file is written for use on Mac OS X.
#

all: apidemo.so

apidemo.so: apidemo.c
	cc -bundle -undefined dynamic_lookup -o apidemo.so apidemo.c -Ilua_src
