PANDORA_SRCDIR?=$(shell cd ../../..; pwd)

all: check

include ${PANDORA_SRCDIR}/Makefile.inc
include ../Makefile.testing

dag-01: dag-01.cpp ${INC}/iface.h ${INC}/dag.h ${LIBNDMGMT}
	${CXX} ${CXXFLAGS} -o dag-01 dag-01.cpp ${LDFLAGS}

check::	dag-01
	./dag-01

checkprograms:: dag-01
programs::

clean::
	@rm -rf dag-01 *.o
