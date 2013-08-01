
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

#define WIN_SIZE 1024
#define TEST_OP ==

int main(int argc, char ** argv)
{
  MPI_Aint win_size = WIN_SIZE;
  MPI_Win win;
  MPI_Group group;
  char* base;
  int disp_unit = 1;
  int rank, size, target_rank, target_disp = 1;
  int r, flag;

  /*************************************************************/
  /* Init and set values */
  /*************************************************************/
  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  target_rank = (rank + 1) % size;
  MPI_Alloc_mem(WIN_SIZE, MPI_INFO_NULL, &base);
  if ( NULL == base )
  {
    printf("failed to alloc %d\n", WIN_SIZE);
    exit(16);
  }


  /*************************************************************/
  /* Win_create */
  /*************************************************************/
  /* MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info,
     MPI_Comm comm, MPI_Win *win); */
  r = MPI_Win_create(base, win_size, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win); 
  if ( MPI_SUCCESS TEST_OP r ) printf("Rank %d failed MPI_Win_create\n", rank);

  /*************************************************************/
  /* First epoch: Tests Put, Get, Get_group, Post, Start,      */
  /*              Complete, Wait, Lock, Unlock                 */
  /*************************************************************/
  r = MPI_Win_get_group(win, &group);
  if ( MPI_SUCCESS TEST_OP r ) printf("Rank %d failed MPI_Win_get_group\n", rank);

  r = MPI_Win_post(group, 0, win);
  if ( MPI_SUCCESS TEST_OP r ) printf("Rank %d failed MPI_Win_post\n", rank);

  r = MPI_Win_start(group, 0, win);
  if ( MPI_SUCCESS TEST_OP r ) printf("Rank %d failed MPI_Win_start\n", rank);

  r = MPI_Win_lock(MPI_LOCK_SHARED, target_rank, 0, win);
  if ( MPI_SUCCESS TEST_OP r ) printf("Rank %d failed MPI_Win_lock\n", rank);

  /* MPI_Put(void *origin_addr, int origin_count, MPI_Datatype
     origin_datatype, int target_rank, MPI_Aint target_disp,
     int target_count, MPI_Datatype target_datatype, MPI_Win win) */
  r = MPI_Put(base, WIN_SIZE, MPI_BYTE, target_rank, target_disp,
     WIN_SIZE, MPI_BYTE, win);
  if ( MPI_SUCCESS TEST_OP r ) printf("Rank %d failed MPI_Put\n", rank);

  r = MPI_Win_unlock(target_rank, win);
  if ( MPI_SUCCESS TEST_OP r ) printf("Rank %d failed MPI_Win_unlock\n", rank);

  /* MPI_Get(void *origin_addr, int origin_count, MPI_Datatype
     origin_datatype, int target_rank, MPI_Aint target_disp,
     int target_count, MPI_Datatype target_datatype, MPI_Win win); */
  r = MPI_Get(base, WIN_SIZE, MPI_BYTE, target_rank, target_disp,
      WIN_SIZE, MPI_BYTE, win);
  if ( MPI_SUCCESS TEST_OP r ) printf("Rank %d failed MPI_Get\n", rank);

  r = MPI_Win_complete(win);
  if ( MPI_SUCCESS TEST_OP r ) 
    printf("Rank %d failed MPI_Win_complete\n", rank);

  r = MPI_Win_test(win, &flag);
  if ( MPI_SUCCESS TEST_OP r ) printf("Rank %d failed MPI_Win_test\n", rank);

  r = MPI_Win_wait(win);
  if ( MPI_SUCCESS TEST_OP r ) printf("Rank %d failed MPI_Win_wait\n", rank);

  /*************************************************************************/
  /* Second epoch: Tests Accumulate and Fence */
  /*************************************************************************/
  r = MPI_Win_fence(0, win);
  if ( MPI_SUCCESS TEST_OP r ) printf("Rank %d failed MPI_Win_fence\n", rank);

  if ( rank == 0 )
  {
    /* MPI_Accumulate(void *origin_addr, int origin_count, MPI_Datatype
       origin_datatype, int target_rank, MPI_Aint target_disp, 
       int target_count, MPI_Datatype target_datatype, 
       MPI_Op op, MPI_Win win) */
    r = MPI_Accumulate(base, WIN_SIZE, MPI_BYTE, 0,
        target_disp, WIN_SIZE, MPI_BYTE, MPI_SUM, win);
    if ( MPI_SUCCESS TEST_OP r ) 
      printf("Rank %d failed MPI_Accumulate\n", rank);
  }
  r = MPI_Win_fence(0, win);
  if ( MPI_SUCCESS TEST_OP r ) printf("Rank %d failed MPI_Win_fence\n", rank);


  /*************************************************************/
  /* Win_free and Finalize */
  /*************************************************************/
  r = MPI_Win_free(&win);
  if ( MPI_SUCCESS TEST_OP r ) printf("Rank %d failed MPI_Win_free\n", rank);

  free(base);

  MPI_Finalize();
}

