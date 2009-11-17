# unstrung master makefile
#
# Copyright (C) 2009 Michael Richardson <mcr@sandelman.ca>
# 
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#


UNSTRUNG_SRCDIR?=$(shell pwd)
export UNSTRUNG_SRCDIR

TERMCAP=
export TERMCAP

default:: programs

srcdir?=$(shell pwd)

-include ${UNSTRUNG_SRCDIR}/Makefile.vendor

SUBDIRS=lib programs testing

include ${UNSTRUNG_SRCDIR}/Makefile.top
include ${UNSTRUNG_SRCDIR}/Makefile.inc

