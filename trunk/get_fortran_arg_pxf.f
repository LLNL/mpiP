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
       integer argc_val

       argc_val = IPXFARGC()

       end subroutine mpipi_get_fortran_argc


       subroutine mpipi_get_fortran_arg(idx, buf_len, buf, len)
       integer idx
       integer buf_len
       character (buf_len) buf
       integer len
       integer ierr

       call PXFGETARG(idx, buf, len, ierr)

       end subroutine mpipi_get_fortran_arg
