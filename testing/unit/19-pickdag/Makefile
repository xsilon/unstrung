PANDORA_SRCDIR?=$(shell cd ../../..; pwd)

include ${PANDORA_SRCDIR}/Makefile.inc
include ../Makefile.testing

19-pickdag: 19-pickdag.cpp ${INC}/iface.h ${LIBS}
	${CXX} ${CXXFLAGS} -o 19-pickdag 19-pickdag.cpp ${LDFLAGS}

checkprograms:: 19-pickdag
programs::

check::	19-pickdag
	@mkdir -p ../OUTPUTS
	@echo PICKDAG19:
	./19-pickdag 2>&1 | tee ../OUTPUTS/19-pickdag-output.raw | diff -B - 19-pickdag-output.txt
	@${TCPDUMP} -t -n -r ../OUTPUTS/19-pickdag.pcap -v | tee ../OUTPUTS/19-pickdag-pcap.raw | diff -B - 19-pickdag-pcap.txt

clean::
	@rm -rf 19-pickdag *.o

update::
	cp ../OUTPUTS/19-pickdag-output.raw 19-pickdag-output.txt
	cp ../OUTPUTS/19-pickdag-pcap.raw   19-pickdag-pcap.txt

