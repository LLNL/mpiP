c --- a simple FORTRAN program to test linking to mpi.


      program simple

      include 'mpif.h'

      integer info
      integer rank

      call mpi_init(info)
      write(*,*) 'successfully called init.'

      call mpi_comm_rank(mpi_comm_world,rank,info)

      call mpi_finalize(info)
      write(*,*) 'successfully called finalize.'

      end

c --- eof
