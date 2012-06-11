c  
c     mpiP MPI Profiler ( http://mpip.sourceforge.net/ )
c  
c     Please see COPYRIGHT AND LICENSE information at the end of this file.
c  
c     ----- 
c  
c     16-waitallf.f -- test use of Waitall
c  
      program waitallf
      include "mpif.h"

      integer  ierr, wsize, rank, starg, rtarg
      integer  istat(MPI_STATUS_SIZE,2)
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

      CALL MPI_WAITALL(2,req,istat,ierr)

      CALL MPI_FINALIZE(ierr)
      print *, 'rank ', rank, ' returned from FINALIZE'

      end

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
c --- EOF
