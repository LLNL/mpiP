# Makefile for MPIP	-*-Makefile-*-
# Copyright (C) 2001 Jeffrey Vetter vetter3@llnl.gov
# $Header$

include ../Defs.mak

CFLAGS	+= -g
LFLAGS	+= -L.. -L${HOME}/AIX/lib
LIBS	+= -lmpiP -lbfd -liberty -lintl

CSRCS =	0-simple.c \
	1-hot-potato.c \
	2-pcontrol.c \
	5-medley.c \
	6-nonblock.c \
	7-nonblock-waitall.c \
	8-test.c \
	9-test-mpip-time.c 
FSRCS =	0-simplef.f 
EXES = $(CSRCS:.c=.exe)
EXES += $(FSRCS:.f=.exe)

all: ${EXES}
	@echo All done.

.SUFFIXES: .exe
.c.exe:
	${CC} ${CFLAGS} ${CPPFLAGS} $< -o $@ ${LFLAGS} ${LIBS}

.C.exe:
	${CXX} ${CFLAGS} ${CPPFLAGS} $< -o $@ ${LFLAGS} ${LIBS}

.f.exe:
	${FC} ${FFLAGS} $< -o $@ ${LFLAGS} ${LIBS}

clean::
	rm -f ${EXES} *.mpiP

include ../Rules.mak

##### EOF
