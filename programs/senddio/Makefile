PANDORA_SRCDIR?=$(shell cd ../..; pwd)

# local install of pcap.h, or make sure you have libpcap-dev installed.
CFLAGS+=${LIBPCAPINC} -g
CXXFLAGS+=${LIBPCAPINC} -g
PROGRAM=senddio
PROGRAMDIR=/sbin
OBJS=senddio.o
LIBS=${LIBNDMGMT} ${LIBOSW} ${LIBNETLINK} ${LIBFAKEIFACE}

include ${PANDORA_SRCDIR}/programs/Makefile.program
