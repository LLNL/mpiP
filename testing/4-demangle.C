
#include <mpi.h>

class comm {
  public:
  comm(int*, char***);
  int doBarriers(MPI_Comm, const int);
  ~comm();
};

comm::comm(int* pargc, char*** pargv)
{
  MPI_Init(pargc, pargv);
  MPI_Barrier(MPI_COMM_WORLD);
}


comm::~comm()
{
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
}


int comm::doBarriers(MPI_Comm com, const int x)
{
  for ( int i = 0; i < x; i++ )
   MPI_Barrier(com);

  return 1;
}

main(int argc, char** argv)
{
  const int x = 10;

  comm *mpi = new comm(&argc, &argv);

  mpi->doBarriers(MPI_COMM_WORLD, x);

  delete mpi;

}

