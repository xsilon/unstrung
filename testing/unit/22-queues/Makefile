PANDORA_SRCDIR?=$(shell cd ../../..; pwd)

all: check

include ${PANDORA_SRCDIR}/Makefile.inc
include ../Makefile.testing

22-queues: 22-queues.cpp ${INC}/iface.h ${INC}/dag.h ${LIBNDMGMT}
	${CXX} ${CXXFLAGS} -o 22-queues 22-queues.cpp ${LDFLAGS}

check::	22-queues
	./22-queues | tee ../OUTPUTS/22-queues.raw | diff - 22-queues-out.txt

checkprograms:: 22-queues
programs::

clean::
	@rm -rf 22-queues *.o
