
include ../../Defs.mak

HEADERS = mt_common.h

SRCS =  01_mt_send-recv_b.c     \
        02_mt_send-recv_nb.c    \
        03_mt_barrier.c         \
        04_mt_dynamic_threads.c \
        mt_common.c             \

LIBS += -lpthread -L../.. -lmpiP
all: $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o 01_mt_send-recv_b.exe 01_mt_send-recv_b.c mt_common.c $(LIBS)
	$(CC) $(CFLAGS) -o 02_mt_send-recv_nb.exe 02_mt_send-recv_nb.c mt_common.c ${LIBS}
	$(CC) $(CFLAGS) -o 03_mt_barrier.exe 03_mt_barrier.c mt_common.c ${LIBS}

clean:
	rm -f SRCS =  01_mt_send-recv_b.exe 02_mt_send-recv_nb.exe 03_mt_barrier.exe *.mpiP *.o
#04_mt_dynamic_threads

