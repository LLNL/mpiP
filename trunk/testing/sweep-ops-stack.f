
c --- a simple FORTRAN program to test stack traces with
c ---  the mpi features that sweep3D uses.

      subroutine do_bcasts(rarray, rdarray)
      include 'mpif.h'
      integer rarray(100)
      double precision rdarray(100)

        call mpi_bcast(rarray,100,MPI_INTEGER,3,MPI_COMM_WORLD,ierr)
        print *,'after broadcast I see ',rarray(1),' in my array.'
  
        call mpi_bcast(rdarray,100,MPI_DOUBLE_PRECISION,3,
     &     MPI_COMM_WORLD,ierr)
        print *,'after double precision broadcast I see ',
     &     rdarray(1),' in my array.'
      return
      end

      subroutine runtest(rank, size)
      integer rank, size
      include 'mpif.h'
      integer info,res,ierr,i,tag,istatus
      integer sarray(100)
      integer rarray(100)

      double precision sdarray(100)
      double precision rdarray(100)
      
      tag = 30

        call mpi_barrier(MPI_COMM_WORLD,ierr)
        print *,'passed barrier.'
  
  
        do i=1,100
           sarray(i) = rank
           rarray(i) = rank
           sdarray(i) = 1. * rank
           rdarray(i) = 1. * rank
        end do
  
  
        call do_bcasts(rarray, rdarray)
        call mpi_barrier(MPI_COMM_WORLD,ierr)
        print *,'passed barrier.'
  
        if((rank.eq.0).or.(rank.eq.2))then
           call mpi_send(sarray,100,MPI_INTEGER,rank+1,tag,
     &        MPI_COMM_WORLD,ierr)
           print *,'sent ', sarray(1), ' to ', rank+1
        else
           call mpi_recv(rarray,100,MPI_INTEGER,rank-1,tag,
     &        MPI_COMM_WORLD,istatus,ierr)
           print *,'recvd ', rarray(1), ' from ', rank-1
        endif
        
        call mpi_barrier(MPI_COMM_WORLD,ierr)
        print *,'passed barrier.'
  
        do i=1,100
           sarray(i) = rank
           rarray(i) = rank
           sdarray(i) = 1. * rank
           rdarray(i) = 1. * rank
        end do
  
        call mpi_allreduce(sarray,rarray,100,MPI_INTEGER,MPI_MAX,
     &     MPI_COMM_WORLD,ierr)
        print *,'allreduce placed ', rarray(1), ' in rarray(1)'
        call mpi_allreduce(sdarray,rdarray,100,MPI_DOUBLE_PRECISION,
     &     MPI_MAX,MPI_COMM_WORLD,ierr)
        print *,'allreduce double p placed ', rdarray(1), 
     &    ' in rarray(1)'
      return
      end


      program simsweep3d
      implicit none

      include 'mpif.h'

      integer info,size,rank,res,ierr,i,tag,istatus
      integer sarray(100)
      integer rarray(100)

      double precision sdarray(100)
      double precision rdarray(100)

      tag = 30

      call mpi_init(info)
      print *,'successfully called init.'

      call mpi_comm_size(MPI_COMM_WORLD, size, info);
      call mpi_comm_rank(MPI_COMM_WORLD, rank, info);
      print *,'size = ', size, ' rank = ', rank

      if(size.ne.4)then
         print *,'use only 4 mpi tasks. exiting.'
         call mpi_abort(MPI_COMM_WORLD,res,ierr)
      endif
  
      call runtest(rank, size)

      call mpi_finalize(info)
      print *,'successfully called finalize.'

      end

c --- eof
