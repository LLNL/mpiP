      program waitallf
      include "mpif.h"

      integer  ierr, wsize, rank, starg, rtarg
      integer  istat(1)
      integer req(2)
      real*8 sendbuf, recvbuf

      CALL MPI_INIT( ierr )
 
      CALL MPI_COMM_RANK( MPI_COMM_WORLD, rank, ierr )
      CALL MPI_COMM_SIZE( MPI_COMM_WORLD, wsize, ierr )

      starg = mod( rank+1, wsize )
      rtarg = mod( rank+wsize-1, wsize )

      print *, 'rank ', rank, ' has starget ', starg
      print *, 'rank ', rank, ' has rtarget ', rtarg

      CALL MPI_IRECV(recvbuf,1,MPI_DOUBLE_PRECISION,
     1    rtarg,1,MPI_COMM_WORLD,req(1),ierr)

      CALL MPI_ISEND(sendbuf,1,MPI_DOUBLE_PRECISION,
     1    starg,1,MPI_COMM_WORLD,req(2),ierr)

      CALL MPI_WAITALL(2,req,istat(1),ierr)

      CALL MPI_FINALIZE(ierr)
      print *, 'rank ', rank, ' returned from FINALIZE'

      end

c --- eof
