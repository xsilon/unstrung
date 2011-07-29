UNSTRUNG_SRCDIR?=$(shell cd ../../..; pwd)

include ${UNSTRUNG_SRCDIR}/Makefile.inc
INC=${UNSTRUNG_SRCDIR}/include

# local install of pcap.h, or make sure you have libpcap-dev installed.
CXXFLAGS+=-I/sandel/include -g
LIBS=${LIBFAKEIFACE} ${LIBNDMGMT} ${LIBOSW} ${LIBNETLINK} ${LIBPCAP}
LDFLAGS+=-L${UNSTRUNG_OBJDIR}/lib/libndmgmt -lndmgmt  ${LIBS}

recv-01: recv-01.cpp ${INC}/iface.h ${LIBS}
	${CXX} ${CXXFLAGS} -o recv-01 recv-01.cpp ${LDFLAGS}

checkprograms:: recv-01
programs::

check::	part01 part02 part03 part04

part01: recv-01
	@mkdir -p ../OUTPUTS
	@echo TEST01: PLAINND
	./recv-01 ../INPUTS/plainnd.pcap | tee ../OUTPUTS/recv-01-output.raw | diff -B - recv-01-output.txt
	tcpdump -t -n -r ../OUTPUTS/recv-01-plainnd.pcap -s 0 | diff -B - recv-01-plainnd-out.txt

part02: recv-01
	@mkdir -p ../OUTPUTS
	@echo TEST02: SENDDIO CREATED
	./recv-01 ../INPUTS/senddio.pcap | tee ../OUTPUTS/recv-02-output.raw | diff -B - recv-02-output.txt

part03: recv-01
	@echo TEST03: SENDDIO-2 CREATED
	./recv-01 ../INPUTS/senddio-2.pcap | tee ../OUTPUTS/recv-03-output.raw | diff -B - recv-03-output.txt

part04: recv-01
	@echo TEST04: SENDDIO-example CREATED
	./recv-01 ../INPUTS/senddio-example.pcap | tee ../OUTPUTS/recv-04-output.raw | diff -B - recv-04-output.txt

clean::
	@rm -rf recv-01 *.o