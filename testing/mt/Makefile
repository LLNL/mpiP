
MPICC ?= mpicc

HEADERS = mt_common.h

SRCS =  01_mt_send-recv_b.c     \
        02_mt_send-recv_nb.c    \
        03_mt_barrier.c         \
        04_mt_dynamic_threads.c \
        mt_common.c             \

CFLAGS = -g -O3


all: $(SRCS) $(HEADERS)
	$(MPICC) $(CFLAGS) -o 01_mt_send-recv_b 01_mt_send-recv_b.c mt_common.c
	$(MPICC) $(CFLAGS) -o 02_mt_send-recv_nb 02_mt_send-recv_nb.c mt_common.c
	$(MPICC) $(CFLAGS) -o 03_mt_barrier 03_mt_barrier.c mt_common.c

clean:
	rm -f SRCS =  01_mt_send-recv_b 02_mt_send-recv_nb 03_mt_barrier 
#04_mt_dynamic_threads

