c  
c     mpiP MPI Profiler ( http://mpip.sourceforge.net/ )
c  
c     Please see COPYRIGHT AND LICENSE information at the end of this file.
c  
c     ----- 
c  
c     sweep-ops-stack.f -- a simple FORTRAN program to test stack traces with
c                          the mpi features that sweep3D uses.
c  


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
      integer info,res,ierr,i,tag
      integer istatus(MPI_STATUS_SIZE)
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
        print *,'before runtest return.'
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

      call mpi_comm_size(MPI_COMM_WORLD, size, info)
      call mpi_comm_rank(MPI_COMM_WORLD, rank, info)
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
c  
c  
c  <license>
c  
c  Copyright (c) 2006, The Regents of the University of California. 
c  Produced at the Lawrence Livermore National Laboratory 
c  Written by Jeffery Vetter and Christopher Chambreau. 
c  UCRL-CODE-223450. 
c  All rights reserved. 
c   
c  This file is part of mpiP.  For details, see http://mpip.sourceforge.net/. 
c   
c  Redistribution and use in source and binary forms, with or without
c  modification, are permitted provided that the following conditions are
c  met:
c   
c  * Redistributions of source code must retain the above copyright
c  notice, this list of conditions and the disclaimer below.
c  
c  * Redistributions in binary form must reproduce the above copyright
c  notice, this list of conditions and the disclaimer (as noted below) in
c  the documentation and/or other materials provided with the
c  distribution.
c  
c  * Neither the name of the UC/LLNL nor the names of its contributors
c  may be used to endorse or promote products derived from this software
c  without specific prior written permission.
c  
c  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
c  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
c  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
c  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OF
c  THE UNIVERSITY OF CALIFORNIA, THE U.S. DEPARTMENT OF ENERGY OR
c  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
c  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
c  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
c  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
c  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
c  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
c  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
c   
c   
c  Additional BSD Notice 
c   
c  1. This notice is required to be provided under our contract with the
c  U.S. Department of Energy (DOE).  This work was produced at the
c  University of California, Lawrence Livermore National Laboratory under
c  Contract No. W-7405-ENG-48 with the DOE.
c   
c  2. Neither the United States Government nor the University of
c  California nor any of their employees, makes any warranty, express or
c  implied, or assumes any liability or responsibility for the accuracy,
c  completeness, or usefulness of any information, apparatus, product, or
c  process disclosed, or represents that its use would not infringe
c  privately-owned rights.
c   
c  3.  Also, reference herein to any specific commercial products,
c  process, or services by trade name, trademark, manufacturer or
c  otherwise does not necessarily constitute or imply its endorsement,
c  recommendation, or favoring by the United States Government or the
c  University of California.  The views and opinions of authors expressed
c  herein do not necessarily state or reflect those of the United States
c  Government or the University of California, and shall not be used for
c  advertising or product endorsement purposes.
c  
c  </license>
c  
c  
C --- EOF
