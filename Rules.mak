# Makefile for MPIP	-*-Makefile-*-
# Copyright (C) 2001 Jeffrey Vetter vetter3@llnl.gov
# $Header$

.c.o:
	${CC} ${CFLAGS} ${CPPFLAGS} -c $< -o $@

.f.o:
	${FC} ${CFLAGS} -c $< -o $@

clean::
	-rm -f $(OBJS) *.log core *~ ${C_TARGET} TAGS

##### EOF
