! -*- Fortran -*-
!
!   @PROLOGUE@
!
!   -----
!
!   Chris Chambreau chcham@llnl.gov
!   Integrated Computing and Communications, LLNL
!   23 Jun 2004
!
!   $Header$
!
!   get_fortran_arg.f -- access fortran command line arguments
!

       subroutine mpipi_get_fortran_argc(argc_val)
       !DEC$ attributes C :: mpipi_get_fortran_argc
       !DEC$ attributes REFERENCE :: argc_val
       integer argc_val

       argc_val = iargc()

       end subroutine mpipi_get_fortran_argc


       subroutine mpipi_get_fortran_arg(idx, buf_len, buf, len)
       !DEC$ attributes C :: mpipi_get_fortran_arg
       !DEC$ attributes VALUE :: idx
       !DEC$ attributes VALUE :: buf_len
       !DEC$ attributes REFERENCE :: buf
       !DEC$ attributes REFERENCE :: len
       integer idx
       integer buf_len
       character (buf_len) buf
       integer len
       integer ierr

       call getarg(idx, buf, len)

       end subroutine mpipi_get_fortran_arg
