PANDORA_SRCDIR?=$(shell cd ../../..; pwd)

include ${PANDORA_SRCDIR}/Makefile.inc
include ../Makefile.testing

selfprefix-13: selfprefix-13.cpp ${INC}/iface.h ${LIBS}
	${CXX} ${CXXFLAGS} -o selfprefix-13 selfprefix-13.cpp ${LDFLAGS}

checkprograms:: selfprefix-13
programs::

check::	selfprefix-13
	@mkdir -p ../OUTPUTS
	@echo SELFPREFIX13:
	./selfprefix-13 2>&1 | tee ../OUTPUTS/selfprefix-13-output.raw | diff -B - selfprefix-13-output.txt

clean::
	@rm -rf selfprefix-13 *.o

update::
	cp ../OUTPUTS/selfprefix-13-output.raw selfprefix-13-output.txt
