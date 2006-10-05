# Makefile for MPIP	-*-Makefile-*-
# Please see license in doc/UserGuide.html
# $Id$

.c.o:
	${CC} ${CFLAGS} ${CPPFLAGS} -c $< -o $@

.f.o:
	${FC} ${CFLAGS} -c $< -o $@

clean::
	-rm -f $(OBJS) *.mpiP *.log core *~ ${C_TARGET} TAGS

##### EOF
