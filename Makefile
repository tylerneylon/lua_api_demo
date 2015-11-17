# Makefile
#
# This file is written for use on Mac OS X.
#

all: api_demo.so

api_demo.so: api_demo.c
	cc -bundle -undefined dynamic_lookup -o api_demo.so api_demo.c -Ilua_src
